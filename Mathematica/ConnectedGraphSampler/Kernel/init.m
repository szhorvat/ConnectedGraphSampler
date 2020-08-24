
If[Not@OrderedQ[{10.0, 2}, {$VersionNumber, $ReleaseNumber}],
  Print["ConnectedGraphSampler requires Mathematica 10.0.2 or later.  Aborting."];
  Abort[]
]

Unprotect["ConnectedGraphSampler`*", "ConnectedGraphSampler`Developer`*"];

Get["ConnectedGraphSampler`ConnectedGraphSampler`"]

(* Protect all package symbols *)
SetAttributes[
  Evaluate@Flatten[Names /@ {"ConnectedGraphSampler`*", "ConnectedGraphSampler`Developer`*"}],
  {Protected, ReadProtected}
]
