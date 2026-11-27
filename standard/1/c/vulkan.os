// standard/1/c/vulkan.os  (loader: vulkan-1). Handles are void*/u64; structs passed as void*.
extern "C:/Windows/System32/vulkan-1.dll" {
    // instance / device
    fn vkCreateInstance(ci: void*, alloc: void*, inst: void**) => int;
    fn vkDestroyInstance(inst: void*, alloc: void*) => void;
    fn vkEnumerateInstanceLayerProperties(n: u32*, props: void*) => int;
    fn vkEnumeratePhysicalDevices(inst: void*, n: u32*, devs: void**) => int;
    fn vkGetPhysicalDeviceProperties(pd: void*, props: void*) => void;
    fn vkGetPhysicalDeviceMemoryProperties(pd: void*, props: void*) => void;
    fn vkGetPhysicalDeviceQueueFamilyProperties(pd: void*, n: u32*, props: void*) => void;
    fn vkGetPhysicalDeviceSurfaceSupportKHR(pd: void*, qf: u32, surf: u64, out: u32*) => int;
    fn vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd: void*, surf: u64, caps: void*) => int;
    fn vkGetPhysicalDeviceSurfaceFormatsKHR(pd: void*, surf: u64, n: u32*, f: void*) => int;
    fn vkGetPhysicalDeviceSurfacePresentModesKHR(pd: void*, surf: u64, n: u32*, m: u32*) => int;
    fn vkCreateDevice(pd: void*, ci: void*, alloc: void*, dev: void**) => int;
    fn vkDestroyDevice(dev: void*, alloc: void*) => void;
    fn vkGetDeviceQueue(dev: void*, qf: u32, qi: u32, q: void**) => void;
    fn vkDestroySurfaceKHR(inst: void*, s: u64, alloc: void*) => void;

    // swapchain
    fn vkCreateSwapchainKHR(dev: void*, ci: void*, alloc: void*, sc: u64*) => int;
    fn vkDestroySwapchainKHR(dev: void*, sc: u64, alloc: void*) => void;
    fn vkGetSwapchainImagesKHR(dev: void*, sc: u64, n: u32*, imgs: u64*) => int;
    fn vkAcquireNextImageKHR(dev: void*, sc: u64, timeout: u64, sem: u64, fence: u64, idx: u32*) => int;
    fn vkQueuePresentKHR(q: void*, info: void*) => int;

    // queue / sync
    fn vkQueueSubmit(q: void*, n: u32, subs: void*, fence: u64) => int;
    fn vkQueueWaitIdle(q: void*) => int;
    fn vkDeviceWaitIdle(dev: void*) => int;
    fn vkCreateSemaphore(dev: void*, ci: void*, alloc: void*, s: u64*) => int;
    fn vkDestroySemaphore(dev: void*, s: u64, alloc: void*) => void;
    fn vkCreateFence(dev: void*, ci: void*, alloc: void*, f: u64*) => int;
    fn vkDestroyFence(dev: void*, f: u64, alloc: void*) => void;
    fn vkWaitForFences(dev: void*, n: u32, f: u64*, all: u32, timeout: u64) => int;
    fn vkResetFences(dev: void*, n: u32, f: u64*) => int;

    // image views / render pass / framebuffers
    fn vkCreateImageView(dev: void*, ci: void*, alloc: void*, v: u64*) => int;
    fn vkDestroyImageView(dev: void*, v: u64, alloc: void*) => void;
    fn vkCreateRenderPass(dev: void*, ci: void*, alloc: void*, rp: u64*) => int;
    fn vkDestroyRenderPass(dev: void*, rp: u64, alloc: void*) => void;
    fn vkCreateFramebuffer(dev: void*, ci: void*, alloc: void*, fb: u64*) => int;
    fn vkDestroyFramebuffer(dev: void*, fb: u64, alloc: void*) => void;

    // command pool / buffers
    fn vkCreateCommandPool(dev: void*, ci: void*, alloc: void*, p: u64*) => int;
    fn vkDestroyCommandPool(dev: void*, p: u64, alloc: void*) => void;
    fn vkAllocateCommandBuffers(dev: void*, ai: void*, out: void**) => int;
    fn vkFreeCommandBuffers(dev: void*, pool: u64, n: u32, cbs: void**) => void;
    fn vkResetCommandBuffer(cb: void*, flags: u32) => int;
    fn vkBeginCommandBuffer(cb: void*, bi: void*) => int;
    fn vkEndCommandBuffer(cb: void*) => int;

    // commands
    fn vkCmdBeginRenderPass(cb: void*, info: void*, contents: u32) => void;
    fn vkCmdEndRenderPass(cb: void*) => void;
    fn vkCmdBindPipeline(cb: void*, bp: u32, pipe: u64) => void;
    fn vkCmdSetViewport(cb: void*, first: u32, n: u32, vp: void*) => void;
    fn vkCmdSetScissor(cb: void*, first: u32, n: u32, sc: void*) => void;
    fn vkCmdBindVertexBuffers(cb: void*, first: u32, n: u32, bufs: u64*, offsets: u64*) => void;
    fn vkCmdBindIndexBuffer(cb: void*, buf: u64, offset: u64, kind: u32) => void;
    fn vkCmdBindDescriptorSets(cb: void*, bp: u32, layout: u64, first: u32, n: u32, sets: u64*, dynCount: u32, dynOffsets: u32*) => void;
    fn vkCmdPushConstants(cb: void*, layout: u64, stages: u32, offset: u32, size: u32, data: void*) => void;
    fn vkCmdDraw(cb: void*, vc: u32, ic: u32, fv: u32, fi: u32) => void;
    fn vkCmdDrawIndexed(cb: void*, ic: u32, inst: u32, fi: u32, vo: int, finst: u32) => void;
    fn vkCmdCopyBuffer(cb: void*, src: u64, dst: u64, n: u32, regions: void*) => void;
    fn vkCmdCopyBufferToImage(cb: void*, buf: u64, img: u64, layout: u32, n: u32, regions: void*) => void;
    fn vkCmdPipelineBarrier(cb: void*, srcStage: u32, dstStage: u32, depFlags: u32, memCount: u32, memBarriers: void*, bufCount: u32, bufBarriers: void*, imgCount: u32, imgBarriers: void*) => void;

    // shaders / pipelines
    fn vkCreateShaderModule(dev: void*, ci: void*, alloc: void*, m: u64*) => int;
    fn vkDestroyShaderModule(dev: void*, m: u64, alloc: void*) => void;
    fn vkCreatePipelineLayout(dev: void*, ci: void*, alloc: void*, l: u64*) => int;
    fn vkDestroyPipelineLayout(dev: void*, l: u64, alloc: void*) => void;
    fn vkCreateGraphicsPipelines(dev: void*, cache: u64, n: u32, ci: void*, alloc: void*, p: u64*) => int;
    fn vkDestroyPipeline(dev: void*, p: u64, alloc: void*) => void;

    // memory / buffers / images
    fn vkAllocateMemory(dev: void*, ai: void*, alloc: void*, mem: u64*) => int;
    fn vkFreeMemory(dev: void*, mem: u64, alloc: void*) => void;
    fn vkMapMemory(dev: void*, mem: u64, offset: u64, size: u64, flags: u32, pp: void**) => int;
    fn vkUnmapMemory(dev: void*, mem: u64) => void;
    fn vkCreateBuffer(dev: void*, ci: void*, alloc: void*, b: u64*) => int;
    fn vkDestroyBuffer(dev: void*, b: u64, alloc: void*) => void;
    fn vkGetBufferMemoryRequirements(dev: void*, b: u64, out: void*) => void;
    fn vkBindBufferMemory(dev: void*, b: u64, mem: u64, offset: u64) => int;
    fn vkCreateImage(dev: void*, ci: void*, alloc: void*, img: u64*) => int;
    fn vkDestroyImage(dev: void*, img: u64, alloc: void*) => void;
    fn vkGetImageMemoryRequirements(dev: void*, img: u64, out: void*) => void;
    fn vkBindImageMemory(dev: void*, img: u64, mem: u64, offset: u64) => int;

    // samplers / descriptors
    fn vkCreateSampler(dev: void*, ci: void*, alloc: void*, s: u64*) => int;
    fn vkDestroySampler(dev: void*, s: u64, alloc: void*) => void;
    fn vkCreateDescriptorSetLayout(dev: void*, ci: void*, alloc: void*, l: u64*) => int;
    fn vkDestroyDescriptorSetLayout(dev: void*, l: u64, alloc: void*) => void;
    fn vkCreateDescriptorPool(dev: void*, ci: void*, alloc: void*, p: u64*) => int;
    fn vkDestroyDescriptorPool(dev: void*, p: u64, alloc: void*) => void;
    fn vkAllocateDescriptorSets(dev: void*, ai: void*, sets: u64*) => int;
    fn vkUpdateDescriptorSets(dev: void*, writeCount: u32, writes: void*, copyCount: u32, copies: void*) => void;
}