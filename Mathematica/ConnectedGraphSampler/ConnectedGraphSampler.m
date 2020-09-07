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
    "CGSSample[degrees, n] generates n biased samples.\n" <>
    "CGSSample[degrees, \"Connected\" -> True] samples only connected graphs.\n" <>
    "CGSSample[degrees, \"MultiEdges\" -> True] allows multi-edges.\n" <>
    "CGSSample[degrees, Exponent -> \[Alpha]] sets the degree affinity exponent.";

CGSSampleProp::usage =
    "CGSSampleProp[degrees, prop, n] generates n random graphs with the given degrees, computes prop[graph] for each, then returns the obtained weighted samples as a WeightedData. Accepts the same options as CGSSample.";

CGSSampleWeights::usage = "CGSSampleWeights[degrees, n] generates n random graphs with the given degrees and returns the logarithms of their sampling weights. Accepts the same options as CGSSample.";

CGSSamplePropRaw::usage = "CGSSamplePropRaw[degrees, prop, n] generates n random graphs with the given degrees, computes value = prop[graph] for each, and returns the result as {value, Log[samplingWeight]} pairs. Accepts the same options as CGSSample.";

CGSToWeightedData::usage = "CGSToWeightedData[rawData] converts a list of {value, Log[samplingWeight]} pairs to a WeightedData expression.";

CGSGraphicalQ::usage = "CGSGraphicalQ[degrees] tests if degrees are realized by a simple graph.";

CGSPotentiallyConnectedQ::usage = "CGSPotentiallyConnectedQ[degrees] tests if degrees are potentially connected.";


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
    LTemplate[
      "ConnectedGraphSampler",
      {
        LClass["ConnectedGraphSampler",
          {
            LFun["setDS", {{Integer, 1, "Constant"} (* degree sequence *)}, "Void"],
            LFun["getDS", {}, {Integer, 1}],
            LFun["seed", {Integer}, "Void"],
            LFun["generateSample", {Real (* alpha *)}, {Integer, 2}],
            LFun["generateConnSample", {Real (* alpha *)}, {Integer, 2}],
            LFun["getEdges", {}, {Integer, 2}],
            LFun["getLogProb", {}, Real],
            LFun["graphicalQ", {}, True | False]
          }
        ],
        LClass["ConnectedGraphSamplerMulti",
          {
            LFun["setDS", {{Integer, 1, "Constant"} (* degree sequence *)}, "Void"],
            LFun["getDS", {}, {Integer, 1}],
            LFun["seed", {Integer}, "Void"],
            LFun["generateSample", {Real (* alpha *)}, {Integer, 2}],
            LFun["generateConnSample", {Real (* alpha *)}, {Integer, 2}],
            LFun["getEdges", {}, {Integer, 2}],
            LFun["getLogProb", {}, Real]
          }
        ]
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
      CompileTemplate[template,
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


(***** Helper functions *****)

cgsTag::usage = "Tag used by throw/catch.";

throw::usage = "throw[val]";
throw[val_] := Throw[val, cgsTag]

catch::usage = "catch[expr]";
SetAttributes[catch, HoldFirst]
catch[expr_] := Catch[expr, cgsTag]

check::usage = "check[val]";
check[val_LibraryFunctionError] := throw[$Failed]
check[$Failed] := throw[$Failed]
check[HoldPattern[LibraryFunction[___][___]]] := throw[$Failed]
check[val_] := val

toGraph[n_, opt : OptionsPattern[]][edges_] := Graph[Range[n], edges + 1, Sequence@@FilterRules[{opt}, Options[Graph]]]


(***** Definitions of package functions *****)

CGSGraphicalQ[degrees : {___Integer}] :=
    catch@Block[{sampler = Make["ConnectedGraphSampler"]},
      If[Max[degrees] >= Length[degrees],
        False,
        sampler@"setDS"[degrees];
        sampler@"graphicalQ"[]
      ]
    ]


CGSPotentiallyConnectedQ[{0}] := True (* special case of a single isolated vertex *)
CGSPotentiallyConnectedQ[degrees : {___Integer ? Positive}] :=
    Total[degrees]/2 >= Length[degrees] - 1 && FreeQ[degrees, 0, {1}]
CGSPotentiallyConnectedQ[_] := False


Options[CGSSample] =
    Join[
      Options[Graph],
      {
        "MultiEdges" -> False,
        "Connected" -> False,
        RandomSeeding -> Automatic,
        Exponent -> 1
      }
    ];
SyntaxInformation[CGSSample] = {"ArgumentsPattern" -> {_, _., OptionsPattern[]}};
CGSSample[degrees_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    catch@Block[{sampler = If[TrueQ@OptionValue["MultiEdges"], Make["ConnectedGraphSamplerMulti"], Make["ConnectedGraphSampler"]]},
      check@sampler@"setDS"[degrees];
      sampler@"seed"[ Replace[OptionValue[RandomSeeding], Automatic :> RandomInteger[2^31-1]] ];
      If[TrueQ@OptionValue["Connected"],
        Table[
          {toGraph[Length[degrees], opt]@check@sampler@"generateConnSample"[OptionValue[Exponent]], sampler@"getLogProb"[]},
          {n}
        ]
        ,
        Table[
          {toGraph[Length[degrees], opt]@check@sampler@"generateSample"[OptionValue[Exponent]], sampler@"getLogProb"[]},
          {n}
        ]
      ]
    ]
CGSSample[degrees_, opt : OptionsPattern[]] := catch@First@check@CGSSample[degrees, 1, opt]


Options[CGSSampleWeights] = {
  "MultiEdges" -> False,
  "Connected" -> False,
  RandomSeeding -> Automatic,
  Exponent -> 1
};
CGSSampleWeights[degrees_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    catch@Block[{sampler = If[TrueQ@OptionValue["MultiEdges"], Make["ConnectedGraphSamplerMulti"], Make["ConnectedGraphSampler"]]},
      check@sampler@"setDS"[degrees];
      sampler@"seed"[ Replace[OptionValue[RandomSeeding], Automatic :> RandomInteger[2^31-1]] ];
      If[TrueQ@OptionValue["Connected"],
        Table[
          check@sampler@"generateConnSample"[OptionValue[Exponent]];
          sampler@"getLogProb"[],
          {n}
        ]
        ,
        Table[
          check@sampler@"generateSample"[OptionValue[Exponent]];
          sampler@"getLogProb"[],
          {n}
        ]
      ]
    ]


Options[CGSSampleProp] = {
  "MultiEdges" -> False,
  "Connected" -> False,
  RandomSeeding -> Automatic,
  Exponent -> 1
};
SyntaxInformation[CGSSampleProp] = {"ArgumentsPattern" -> {_, _, _, OptionsPattern[]}};
CGSSampleProp[degrees_, prop_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    CGSToWeightedData@CGSSamplePropRaw[degrees, prop, n, opt]


Options[CGSSamplePropRaw] = {
  "MultiEdges" -> False,
  "Connected" -> False,
  RandomSeeding -> Automatic,
  Exponent -> 1
};
SyntaxInformation[CGSSamplePropRaw] = {"ArgumentsPattern" -> {_, _, _, OptionsPattern[]}};
CGSSamplePropRaw[degrees_, prop_, n_Integer ? NonNegative, opt : OptionsPattern[]] :=
    catch@Block[{sampler = If[TrueQ@OptionValue["MultiEdges"], Make["ConnectedGraphSamplerMulti"], Make["ConnectedGraphSampler"]]},
      check@sampler@"setDS"[degrees];
      sampler@"seed"[ Replace[OptionValue[RandomSeeding], Automatic :> RandomInteger[2^31-1]] ];
      If[TrueQ@OptionValue["Connected"],
        Table[
          {prop@toGraph[Length[degrees]]@check@sampler@"generateConnSample"[OptionValue[Exponent]], sampler@"getLogProb"[]},
          {n}
        ]
        ,
        Table[
          {prop@toGraph[Length[degrees]]@check@sampler@"generateSample"[OptionValue[Exponent]], sampler@"getLogProb"[]},
          {n}
        ]
      ]
    ]


CGSToWeightedData[data_] :=
    Module[{weights, values},
      {values, weights} = Transpose[data];
      weights = weights - Min[weights];
      WeightedData[values, Exp[-weights]]
    ]


End[] (* `Private` *)

EndPackage[]
