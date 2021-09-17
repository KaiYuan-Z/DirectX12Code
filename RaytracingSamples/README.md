# 光栅管线的Demos

注：读研的时候光追AO的实验做的比较多，所以目前基本都是光追AO相关的实验Demo。

## 1. RayTracingAO_001(RasterizerGBuffer_Halton_Cosine)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：是
* 引用Lib：Core、Effects、RaytracingUtility
* 说明：基本光追框架演示、使用RasterizerGBuffer的光追AO、测试RaytracingUtility的Halton随机样本支持、均匀采样和Cosine重要性采样对比
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/RayTracingAO_RasterizerGBuffer_Halton_Cosine.png" width="960"/></div>

<br>

<br>

## 2. RayTracingAO_002(RasterizerGBuffer_HaltonOrMultiJitter)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：是
* 引用Lib：Core、Effects、RaytracingUtility
* 用途：对比本框架使用的基于Halton的Stratification性质的随机样本和基于MultiJitter的随机样本
* 说明：UI上的开关OpenMultiJitter勾选使用的就是MultiJitter，否则就是本框架内置的Halton的。
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/RayTracingAO_RasterizerGBuffer_HaltonOrMultiJitter.png" width="960"/></div>

<br>

<br>

## 3. RayTracingAO_003(RaytracedGBuffer_MultiRaysPerPixel_SimpleHalton)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：是
* 引用Lib：Core、Effects、RaytracingUtility
* 用途：测试Halton随机样本，索引按固定步长做的偏移，没有使用Stratification性质；光追的G-Buffer+光追的AO
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/RayTracingAO_RaytracedGBuffer_MultiRaysPerPixel_SimpleHalton.png" width="960"/></div>

<br>

<br>

## 4. TemporalRayTracingAO_001(SimpleAccumFrame_RaytracedGBuffer)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：是
* 引用Lib：Core、Effects、RaytracingUtility
* 用途：基于时间复用的光追AO
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/TemporalRayTracingAO_SimpleAccumFrame_RaytracedGBuffer.png" width="960"/></div>

<br>

<br>

## 5. TemporalRayTracingAO_002(CrossroadDemo)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：是
* 引用Lib：Core、Effects、RaytracingUtility
* 用途：毕设的Demo场景，具体介绍可参看[毕设论文.PDF](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/RaytracingSamples/TemporalRayTracingAO_002/PDF)，Demo视频可参看[Demo演示](https://pan.baidu.com/s/1R9pzV-kC0yY_CyeSSJm1kw)
* 说明：<br>
采样：历史复用低时8 spp，历史复用中等时4 spp，历史复用高时1 spp <br>
F1：展示当前帧的AO <br>
F2：展示最终的AO <br>
F3：展示最终场景（光追AO+光栅的Deferred光照） <br>
O：开闭AO <br>
1：开了基于AO变化调整复用 <br>
2：关了基于AO变化调整复用 <br>
B：展示动态物体的AO包围盒 <br>
N：展示前后两帧AO变化（越亮变化越大）、以及屏幕各处AO采样数（红：8 spp，绿：4 spp，蓝：1 spp） <br>
M：让场景动起来或者静止 <br>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/TemporalRayTracingAO_CrossroadDemo2.png" width="960"/></div>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/TemporalRayTracingAO_CrossroadDemo1.png" width="960"/></div>

