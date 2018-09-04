//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer cbPerObject
// {
//
//   float4x4 gWorldViewProj;           // Offset:    0 Size:    64
//   float4 gPulseColor;                // Offset:   64 Size:    16 [unused]
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// cbPerObject                       cbuffer      NA          NA            cb0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// POSITION                 0   xyz         0     NONE   float   xyz 
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float   xyzw
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
vs_5_0
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_constantbuffer CB0[4], immediateIndexed
dcl_input v0.xyz
dcl_input v1.xyzw
dcl_output_siv o0.xyzw, position
dcl_output o1.xyzw
dcl_temps 2
//
// Initial variable locations:
//   v0.x <- vin.PosL.x; v0.y <- vin.PosL.y; v0.z <- vin.PosL.z; 
//   v1.x <- vin.Color.x; v1.y <- vin.Color.y; v1.z <- vin.Color.z; v1.w <- vin.Color.w; 
//   o1.x <- <VS return value>.Color.x; o1.y <- <VS return value>.Color.y; o1.z <- <VS return value>.Color.z; o1.w <- <VS return value>.Color.w; 
//   o0.x <- <VS return value>.PosH.x; o0.y <- <VS return value>.PosH.y; o0.z <- <VS return value>.PosH.z; o0.w <- <VS return value>.PosH.w
//
#line 34 "E:\Projects\Visual Community 2017\DirectX12_Learning\DirectX12_Learning\DirectX12_Learning\Shaders\simpleVS_01.hlsl"
mov r0.xyz, v0.xyzx
mov r0.w, l(1.000000)
dp4 r1.x, r0.xyzw, cb0[0].xyzw  // r1.x <- vout.PosH.x
dp4 r1.y, r0.xyzw, cb0[1].xyzw  // r1.y <- vout.PosH.y
dp4 r1.z, r0.xyzw, cb0[2].xyzw  // r1.z <- vout.PosH.z
dp4 r1.w, r0.xyzw, cb0[3].xyzw  // r1.w <- vout.PosH.w

#line 36
mov r0.xyzw, v1.xyzw  // r0.x <- vout.Color.x; r0.y <- vout.Color.y; r0.z <- vout.Color.z; r0.w <- vout.Color.w

#line 37
mov o0.xyzw, r1.xyzw
mov o1.xyzw, r0.xyzw
ret 
// Approximately 10 instruction slots used
