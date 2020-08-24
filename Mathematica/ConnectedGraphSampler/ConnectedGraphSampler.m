(* Mathematica Package *)
(* Based on LTemplate, https://github.com/szhorvat/LTemplate *)

(* :Title: ConnectedGraphSampler *)
(* :Context: ConnectedGraphSampler` *)
(* :Author: Szabolcs Horvát *)
(* :Date: 2020-08-24 *)

(* :Package Version: 0.1 *)
(* :Mathematica Version: 10.0 *)
(* :Copyright: (c) 2020 Szabolcs Horvát *)
(* :Keywords: *)
(* :Discussion: *)

BeginPackage["ConnectedGraphSampler`"]

(* Privately load LTemplate. Note the leading ` character which ensures that the embedded LTemplate gets loaded. *)
Get["LTemplate`LTemplatePrivate`"];

(* ConfigureLTemplate[] *must* be called at this point. *)
ConfigureLTemplate[
  "MessageSymbol" -> ConnectedGraphSampler,
  (* If lazy loading is enabled, functions are loaded only on first use.
     This improves package loading performance, but it is not convenient
     during development and debugging. *)
  "LazyLoading" -> False
];

(* Public package symbols go here: *)

ConnectedGraphSampler::usage = "ConnectedGraphSampler is a symbol to which package messages are associated.";

CGSSample::usage =
    "CGSSample[degrees] generates a biased sample from the set of graphs with the given degrees. The result is returned in the form {graph, Log[samplingWeight]}.\n" <>
    "CGSSample[degrees, Connected -> True] samples only connected graphs.\n" <>
    "CGSSample[degrees, n] generates n biased samples.";
CGSSampleProp::usage =
    "CGSSampleProp[degrees, prop, n] generates n random graphs of the given degrees, computed prop[graph] for each, then returns the obtained weighted samples as a WeightedData. Use Connected -> True to sample only connected graphs.";
CGSGraphicalQ::usage = "CGSGraphicalQ[degrees] tests if degrees are realized by a simple graph.";
CGSPotentiallyConnectedQ::usage = "CGSPotentiallyConnectedQ[degrees] tests if degrees are potentially connected.";
Connected


`Developer`Recompile::usage = "ConnectedGraphSampler`Developer`Recompile[] recompiles the ConnectedGraphSampler library and reloads the functions.";
PrependTo[$ContextPath, $Context <> "Developer`"];


Begin["`Private`"]

(* Helper function: abort package loading and leave a clean $ContextPath behind *)
packageAbort[] := (End[]; EndPackage[]; Abort[])

$packageVersion    = "0.1";
$packageDirectory  = DirectoryName[$InputFileName];

$systemID = $SystemID;

(* On OS X libraries use libc++ ABI since M10.4 and libstdc++ ABI up to M10.3.
   We need separate binaries to support M10.3 and earlier.
   http://community.wolfram.com/groups/-/m/t/816033 *)
If[$OperatingSystem === "MacOSX", $systemID = $systemID <> If[$VersionNumber <= 10.3, "-libstdc++", "-libc++"]];

$libraryDirectory  = FileNameJoin[{$packageDirectory, "LibraryResources", $systemID}];
$sourceDirectory   = FileNameJoin[{$packageDirectory, "LibraryResources", "Source"}];
$buildSettingsFile = FileNameJoin[{$packageDirectory, "BuildSettings.m"}];

(* Add $libraryDirectory to $LibraryPath in case package is not installed in $UserBaseDirectory/Applications. *)
If[Not@MemberQ[$LibraryPath, $libraryDirectory],
  PrependTo[$LibraryPath, $libraryDirectory]
]


(***** The library template *****)

template =
    LClass["ConnectedGraphSampler",
      {
        LFun["setDS", {{Integer, 1, "Constant"}}, "Void"],
        LFun["getDS", {}, {Integer, 1}],
        LFun["seed", {Integer}, "Void"],
        LFun["generateSample", {}, {Integer, 2}],
        LFun["generateConnSample", {}, {Integer, 2}],
        LFun["getEdges", {}, {Integer, 2}],
        LFun["getLogProb", {}, Real],
        LFun["graphicalQ", {}, True|False]
      }
    ];


(***** Compilation, loading and initialization *****)

$buildSettings = None;
If[FileExistsQ[$buildSettingsFile], Get[$buildSettingsFile] ]


Recompile::build = "No build settings found. Please check BuildSettings.m."

Recompile[] :=
    Module[{},
      (* abort compilation if no build settings are present *)
      If[$buildSettings === None,
        Message[Recompile::build];
        Return[$Failed]
      ];
      (* create directory for binary if it doesn't exist yet *)
      If[Not@DirectoryQ[$libraryDirectory],
        CreateDirectory[$libraryDirectory]
      ];
      (* compile code *)
      SetDirectory[$sourceDirectory];
      CompileTemplate[template, { (* TODO add any extra .cpp source files to be included in the compilation *) },
        "ShellCommandFunction" -> Print, "ShellOutputFunction" -> Print,
        "TargetDirectory" -> $libraryDirectory,
        Sequence @@ $buildSettings
      ];
      ResetDirectory[];
      (* load library *)
      loadConnectedGraphSampler[] (* defined below *)
    ]


loadConnectedGraphSampler[] :=
    Module[{deps},
      (* mechanism for loading shared library dependencies, if necessary *)
      deps = FileNameJoin[{$libraryDirectory, "dependencies.m"}];
      Check[
        If[FileExistsQ[deps], Get[deps]],
        Return[$Failed]
      ];
      (* load the library *)
      If[LoadTemplate[template] === $Failed,
        Return[$Failed]
      ];
      (* TODO add any other post-loading initialization if necessary *)
    ]


(* Load library, compile if necessary. *)
If[LoadTemplate[template] === $Failed,
  Print[Style["Loading failed, trying to recompile ...", Red]];
  If[Recompile[] === $Failed
    ,
    Print[Style["Cannot load or compile library. \[SadSmiley] Aborting.", Red]];
    packageAbort[] (* cleanly abort package loading *)
    ,
    Print[Style["Successfully compiled and loaded the library. \[HappySmiley]", Red]];
  ]
]


(***** Definitions of package functions *****)

cgsTag::usage = "";

throw::usage = "throw[val]";
throw[val_] := Throw[val, cgsTag]

catch::usage = "catch[expr]";
SetAttributes[catch, HoldFirst]
catch[expr_] := Catch[expr, cgsTag]

check::usage = "check[val]";
check[val_LibraryFunctionError] := throw[$Failed] (* this was originally throw[val] *)
check[$Failed] := throw[$Failed]
check[HoldPattern[LibraryFunction[___][___]]] := throw[$Failed]
check[val_] := val

toGraph[n_, opt : OptionsPattern[]][edges_] := Graph[Range[n], edges + 1, Sequence@@FilterRules[{opt}, Options[Graph]]]

CGSGraphicalQ[degrees : {___Integer}] :=
    catch@Block[{sampler = Make["ConnectedGraphSampler"]},
      If[Max[degrees] >= Length[degrees],
        False,
        sampler@"setDS"[degrees];
        sampler@"graphicalQ"[]
      ]
    ]

CGSPotentiallyConnectedQ[degrees : {___Integer}] :=
    Total[degrees]/2 >= Length[degrees] - 1

Options[CGSSample] =
    Join[
      Options[Graph],
      { Connected -> False }
    ];
SyntaxInformation[CGSSample] = {"ArgumentsPattern" -> {_, _., OptionsPattern[]}};
CGSSample[degrees_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    catch@Block[{sampler = Make["ConnectedGraphSampler"]},
      check@sampler@"setDS"[degrees];
      If[OptionValue[Connected],
        Table[
          {toGraph[Length[degrees], opt]@check@sampler@"generateConnSample"[], sampler@"getLogProb"[]},
          {n}
        ]
        ,
        Table[
          {toGraph[Length[degrees], opt]@check@sampler@"generateSample"[], sampler@"getLogProb"[]},
          {n}
        ]
      ]
    ]
CGSSample[degrees_, opt : OptionsPattern[]] := catch@First@check@CGSSample[degrees, 1, opt]

Options[CGSSampleProp] = {
  Connected -> False
};
SyntaxInformation[CGSSampleProp] = {"ArgumentsPattern" -> {_, _, _, OptionsPattern[]}};
CGSSampleProp[degrees_, prop_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    catch@Module[{sampler = Make["ConnectedGraphSampler"], values, weights},
      check@sampler@"setDS"[degrees];
      {values, weights} = Transpose@If[OptionValue[Connected],
        Table[
          {prop@toGraph[Length[degrees]]@check@sampler@"generateConnSample"[], sampler@"getLogProb"[]},
          {n}
        ]
        ,
        Table[
          {prop@toGraph[Length[degrees]]@check@sampler@"generateSample"[], sampler@"getLogProb"[]},
          {n}
        ]
      ];
      weights = weights - Min[weights];
      WeightedData[values, Exp[-weights]]
    ]

End[] (* `Private` *)

EndPackage[]