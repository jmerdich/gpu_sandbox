#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC  1
#include "vulkan/vulkan.hpp"
#include <iostream>
#include <chrono>
#if _WIN32
#include <profileapi.h>
#include <synchapi.h>
#endif
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

uint32_t findMemoryType( vk::PhysicalDeviceMemoryProperties const & memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask, vk::MemoryPropertyFlags antiReqMask )
{
    uint32_t typeIndex = uint32_t( ~0 );
    for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ )
    {
    if ( ( typeBits & 1 ) && ( ( memoryProperties.memoryTypes[i].propertyFlags & requirementsMask ) == requirementsMask ) && (( memoryProperties.memoryTypes[i].propertyFlags & antiReqMask ) == vk::MemoryPropertyFlags(0) ))
    {
        typeIndex = i;
        break;
    }
    typeBits >>= 1;
    }
    assert( typeIndex != uint32_t( ~0 ) );
    return typeIndex;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                                                VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                                                                VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                                                                void * /*pUserData*/ )
    {
#if !defined( NDEBUG )
      if ( static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0x822806fa )
      {
        // Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils, but this extension is intended to support use by applications when
        // debugging and it is strongly recommended that it be otherwise avoided.
        return VK_FALSE;
      }
      else if ( static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0xe8d1a9fe )
      {
        // Validation Performance Warning: Using debug builds of the validation layers *will* adversely affect performance.
        return VK_FALSE;
      }
#endif

      std::cerr << vk::to_string( static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ) ) << ": "
                << vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) ) << ":\n";
      std::cerr << std::string( "\t" ) << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
      std::cerr << std::string( "\t" ) << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
      std::cerr << std::string( "\t" ) << "message         = <" << pCallbackData->pMessage << ">\n";
      if ( 0 < pCallbackData->queueLabelCount )
      {
        std::cerr << std::string( "\t" ) << "Queue Labels:\n";
        for ( uint32_t i = 0; i < pCallbackData->queueLabelCount; i++ )
        {
          std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
      }
      if ( 0 < pCallbackData->cmdBufLabelCount )
      {
        std::cerr << std::string( "\t" ) << "CommandBuffer Labels:\n";
        for ( uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ )
        {
          std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
      }
      if ( 0 < pCallbackData->objectCount )
      {
        std::cerr << std::string( "\t" ) << "Objects:\n";
        for ( uint32_t i = 0; i < pCallbackData->objectCount; i++ )
        {
          std::cerr << std::string( "\t\t" ) << "Object " << i << "\n";
          std::cerr << std::string( "\t\t\t" ) << "objectType   = " << vk::to_string( static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType ) )
                    << "\n";
          std::cerr << std::string( "\t\t\t" ) << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
          if ( pCallbackData->pObjects[i].pObjectName )
          {
            std::cerr << std::string( "\t\t\t" ) << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
          }
        }
      }
      return VK_FALSE;
    }

int main() {
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    vk::ApplicationInfo appInfo { "CopyBenches Demo", 1, "none", 1, VK_API_VERSION_1_2};
    vk::InstanceCreateInfo instanceInfo { {}, &appInfo };

    const char* enabledInstLayers[] = { "VK_LAYER_KHRONOS_validation"};
    const char* enabledInstExts[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME  };
    instanceInfo.setPpEnabledLayerNames(&enabledInstLayers[0]);
    instanceInfo.setEnabledLayerCount(sizeof(enabledInstLayers)/sizeof(enabledInstLayers[0]));
    instanceInfo.setPpEnabledExtensionNames(enabledInstExts);
    instanceInfo.setEnabledExtensionCount(sizeof(enabledInstExts)/sizeof(enabledInstExts[0]));

    auto instance = vk::createInstanceUnique(instanceInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init( *instance );
    auto debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique( 
              { {},
               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
               vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
               &debugUtilsMessengerCallback }
     );

    vk::PhysicalDevice physicalDevice = instance->enumeratePhysicalDevices().front();

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamiliyProperties which supports dma only (SDMA)
    auto   propertyIterator         = std::find_if( queueFamilyProperties.begin(),
                                          queueFamilyProperties.end(),
                                          []( vk::QueueFamilyProperties const & qfp ) { return (qfp.queueFlags & vk::QueueFlagBits::eTransfer) && !(qfp.queueFlags & vk::QueueFlagBits::eCompute); } );
    uint32_t dmaQueueFamilyIndex = (uint32_t)std::distance( queueFamilyProperties.begin(), propertyIterator );
    assert( dmaQueueFamilyIndex < queueFamilyProperties.size() );

    // create a Device
    float                     queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo( vk::DeviceQueueCreateFlags(), static_cast<uint32_t>( dmaQueueFamilyIndex ), 1, &queuePriority );
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceHostQueryResetFeatures> devCreateInfo(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo), {true});

    auto device = physicalDevice.createDeviceUnique(devCreateInfo.get<vk::DeviceCreateInfo>());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    // create to/from memory

    size_t copySize = 64 * 1024 * 1024;
    auto copyTestBufferSrc = device->createBufferUnique( vk::BufferCreateInfo(
      vk::BufferCreateFlags(), copySize, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc ) );
    auto copyTestBufferDst = device->createBufferUnique( vk::BufferCreateInfo(
      vk::BufferCreateFlags(), copySize, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc ) );

    vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements( *copyTestBufferSrc ); // Same for both

    uint32_t memoryTypeIndexDst = findMemoryType(physicalDevice.getMemoryProperties(),
                                              memoryRequirements.memoryTypeBits,
                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal,
                                              vk::MemoryPropertyFlags());
    uint32_t memoryTypeIndexSrc = findMemoryType(physicalDevice.getMemoryProperties(),
                                              memoryRequirements.memoryTypeBits,
                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached,
                                              vk::MemoryPropertyFlagBits::eDeviceLocal);
    // memoryTypeIndexDst = memoryTypeIndexSrc;

    auto copyTestMemorySrc  = device->allocateMemoryUnique( vk::MemoryAllocateInfo( copySize, memoryTypeIndexSrc ) );
    auto copyTestMemoryDst  = device->allocateMemoryUnique( vk::MemoryAllocateInfo( copySize, memoryTypeIndexDst ) );

    device->bindBufferMemory(*copyTestBufferSrc, *copyTestMemorySrc, 0);
    device->bindBufferMemory(*copyTestBufferDst, *copyTestMemoryDst, 0);

    // create timestamp query pool
    auto queryPool = device->createQueryPoolUnique(vk::QueryPoolCreateInfo( vk::QueryPoolCreateFlags(), vk::QueryType::eTimestamp, 2, vk::QueryPipelineStatisticFlags() ) );

    // Create a cmdbuf
    auto   commandPool = device->createCommandPoolUnique( { {}, dmaQueueFamilyIndex } );
    auto commandBuffer =
      std::move(device->allocateCommandBuffersUnique( vk::CommandBufferAllocateInfo( *commandPool, vk::CommandBufferLevel::ePrimary, 1 ) ).front());

    vk::Queue dmaQueue = device->getQueue(dmaQueueFamilyIndex, 0);
    auto dmaFence = device->createFenceUnique( {} );

    // core loop start

    device->resetQueryPool(*queryPool, 0, 2);
    commandBuffer->begin(vk::CommandBufferBeginInfo() );

    commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *queryPool, 0);
    int copyCount = 200;
    vk::BufferCopy copyInfo = {0, 0, copySize};
    for (int i = 0; i < copyCount; i++)
        commandBuffer->copyBuffer(*copyTestBufferSrc, *copyTestBufferDst, copyInfo);

    commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *queryPool, 1);

    // Just in case
    // commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe, {}, nullptr, nullptr, nullptr);

    
    commandBuffer->end();

    vk::PipelineStageFlags nullFlags {};
    dmaQueue.submit( vk::SubmitInfo( 0, nullptr,  &nullFlags, 1, &*commandBuffer ), *dmaFence );
    dmaQueue.waitIdle();

    while ( device->waitForFences( *dmaFence, true, 1'000'000 ) == vk::Result::eTimeout )
      ;

    uint64_t  pTimes[2] = {};
    device->getQueryPoolResults(*queryPool, 0, 2, sizeof( pTimes), &pTimes[0], sizeof(uint64_t),  vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait );

    uint64_t elapsedTime = pTimes[1] - pTimes[0];
    auto props = physicalDevice.getProperties();
    uint64_t timestep = props.limits.timestampPeriod;

    float elapsedMs = ((float)elapsedTime*timestep)/1'000'000;
    float copySizeMB = (copySize/(1*1024*1024)) * copyCount;
    float rateMBps = (copySizeMB/elapsedMs)*1000;

    printf("Elapsed: %fms\n", elapsedMs);
    printf("SDMA Copied %fMB at %f MB/s\n", copySizeMB, rateMBps);

    void* pSrcMem = nullptr;
    void* pDstMem = nullptr;
    device->mapMemory(*copyTestMemorySrc, 0, copySize, vk::MemoryMapFlags(), &pSrcMem);
    device->mapMemory(*copyTestMemoryDst, 0, copySize, vk::MemoryMapFlags(), &pDstMem);

    copyCount = 50;

    assert((copySize % ((128/8)* 8)) == 0 );
    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < copyCount; i++) {
#if 1
      __m256i* pCurSrc = (__m256i*)pSrcMem;
      __m256i* pCurDst = (__m256i*)pDstMem;
      __m256i* pEnd = (__m256i*)(((char*)pSrcMem) + copySize);
      while (pCurSrc != pEnd) {
        __m256i t0 = _mm256_load_si256(pCurSrc++);
        __m256i t1 = _mm256_load_si256(pCurSrc++);
        __m256i t2 = _mm256_load_si256(pCurSrc++);
        __m256i t3 = _mm256_load_si256(pCurSrc++);
        __m256i t4 = _mm256_load_si256(pCurSrc++);
        __m256i t5 = _mm256_load_si256(pCurSrc++);
        __m256i t6 = _mm256_load_si256(pCurSrc++);
        __m256i t7 = _mm256_load_si256(pCurSrc++);
        _mm256_stream_si256(pCurDst++, t0);
        _mm256_stream_si256(pCurDst++, t1);
        _mm256_stream_si256(pCurDst++, t2);
        _mm256_stream_si256(pCurDst++, t3);
        _mm256_stream_si256(pCurDst++, t4);
        _mm256_stream_si256(pCurDst++, t5);
        _mm256_stream_si256(pCurDst++, t6);
        _mm256_stream_si256(pCurDst++, t7);
      }
#else
      memcpy(pDstMem, pSrcMem, copySize);
#endif
    }
    const auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    elapsedMs = elapsed.count();

    copySizeMB = (copySize/(1*1024*1024)) * copyCount;
    rateMBps = (copySizeMB/elapsedMs)*1000;

    printf("Elapsed: %fms\n", elapsedMs);
    printf("CPU Copied %fMB at %f MB/s\n", copySizeMB, rateMBps);

    return 0;
}