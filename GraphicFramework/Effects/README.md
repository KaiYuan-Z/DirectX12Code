
# Effects说明

<br>

<br>

## 简述
* Effects是常用图形算法库，在这个库中常用的图形算法都被封装成了一个个单独的Effect，以方便在各个Demo程序间复用。<br>
* 使用Effects中的算法时，只需要引用Effects的Lib，然后包含对应Effect的头文件即可。<br>
* Effect已经将一个算法的绘制逻辑和Shader都封装好，不需要Demo程序再编写算法相关的Shader或Render代码。<br>
* 在Demo中使用Effect时，一般都是在Init阶段初始化一下Effect对象，然后在每帧直接调用Effect的Render函数即可。<br>

<br>

<br>

## Effect目录

* ImageEffect：用来输出RT到屏幕，调试时常用到。

<br>

* GBufferEffect：用于计算场景G-Buffer，支持BlinnPhong和PBR。

<br>

* GeometryOnlyGBufferEffect：只为场景计算G-Buffer中的几何数据，用于写AO的Demo程序。

<br>

* PartialDerivativesEffect：用来计算每个像素的偏导，既计算ddx和ddy。

<br>

* MeanVarianceEffect：用来计算每个像素的局部方差。

<br>

* DownSampleEffect：降采样的Effect。

<br>

* DecodeNormalEffect：法线解码的Effect，目前Core的G-Buffer是2通道float，调试算法时某些代码若是未压缩的法线可以用它来解码。

<br>

* SkyEffect：天空盒。

<br>

* ShadowMapEffect：简单的ShadowMap算法。

<br>

* SsaoEffect：简单的SSAO算法。

<br>

* HBAOPlusEffect：Nvidia的HBAO PLUS。HBAO PLUS dll的源码来自[Nvidia GameWorks HBAOPlus](https://github.com/NVIDIAGameWorks/HBAOPlus)。

<br>

* FxaaEffect：Nvidia的FXAA。

<br>

* TemporalEffect：TAA反走样。

<br>

* BlurEffect：简单高斯滤波，内部的1/2半径采样步长为1，外部的1/2半径采样步长为2，边界检测只用了深度。参考了《Stable SSAO in Battlefield 3 with Selective Temporal Filtering》。

<br>

* BilateralBlurEffect：高斯滤波，边界函数比BlurEffect的更复杂（多了ddxy和法线，检测更准确），其他同BlurEffect。

<br>

* AtrousWaveletTransfromBlurEffect：参考了SVGF的第一趟滤波。做实时光追AO时，感觉Value Based Weight（用来避免Sharp Value被过度Blur）对AO来说作用不大，暂时把Value Based Weight去掉了(去掉了Value Based Weight，感觉Blur算法的名字将来也得换换，暂时还没来得及改)，这样还省得算方差了。未来其他算法有需要Value Based Weight的话还会加回来，然后为AO但写一个不带Value Based Weight的。

<br>

* SimpleAccumulationEffect：最简单的复用历史帧的Effect，相机一动累积计数就会清零，仅用于简单验证一下时间复用的效果

<br>

* AccumulationEffect：带重投影和基于深度的有效性检测的复用历史帧的Effect，重投影不支持形变模型，每次重投影只采历史帧中一个像素。由于每次重投影只采一个像素，有效性检测严了一动起来物体边界就容易出噪声，有效性检测松了一动物起来体边界就容易糊

<br>

* AccumulationEffectTestID：与AccumulationEffect相比多了Geometry ID检测的复用历史帧的Effect。由于多了基于Geometry ID的有效性检测，在物体边界间的有些过于严苛了，导致一动起来物体边界就更容易出噪声。其实，有时候有效性检测弄得松一点，让边界在动起来的情况下稍微糊一点，反而能平滑因历史样本失效而出现的噪声。

<br>

* RayTracedAoAccumulationEffect：用于RayTracedAo的复用历史帧的Effect，和AccumulationEffect相比多了个根据新一帧的AO结果Clamp历史结果的操作（类似TAA的Clamp操作）。AO的每帧采样数比较高时（8 SPP 以上）效果不错，每帧采样数采样数在4 SPP还凑合，每帧采样数采样数只有1-2 SPP时 Clamp效果就比较垃圾。Clamp操作对当前帧的采样数有一定要求，不太适合对性能要求比较苛刻的实时光追AO。

<br>

* RayTracedAoTemporalSupersamplingEffect：用于RayTracedAo的复用历史帧的Effect，重投影时每个像素会采周围4个历史像素（运动时，物体边界不容易糊，也不容易出现明显噪声），为了支持每帧低AO采样数（每帧1 SPP）没有使用TAA的Clamp，而是使用了AoDiff（前后两帧的AO变化率，具体介绍可参看[毕设论文.PDF](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/RaytracingSamples/TemporalRayTracingAO_002/PDF)）来调整AO复用。

<br>

* CalcAoDiffEffect：计算前后两帧的AO变化率，用来调整AO复用。具体介绍可参看[毕设论文.PDF](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/RaytracingSamples/TemporalRayTracingAO_002/PDF)）。

<br>

* MarkDynamicEffect：标记动态物体的AO影响范围，用来指导AoDiff的计算（不动的物体不需要算AoDiff，MarkDynamicEffect可以帮助剔除不必要的AoDiff计算）。具体介绍可参看[毕设论文.PDF](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/RaytracingSamples/TemporalRayTracingAO_002/PDF)）
