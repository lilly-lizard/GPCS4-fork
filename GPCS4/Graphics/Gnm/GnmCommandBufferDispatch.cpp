#include "GnmCommandBufferDispatch.h"
#include "GnmInitializer.h"
#include "GnmGpuLabel.h"
#include "GnmBuffer.h"
#include "GnmTexture.h"
#include "Violet/VltDevice.h"
#include "Violet/VltContext.h"
#include "Violet/VltCmdList.h"
#include "Sce/SceGpuQueue.h"
#include "Sce/SceLabelManager.h"

#include <stdexcept>

using namespace sce::vlt;
using namespace sce::gcn;

namespace sce::Gnm
{

	GnmCommandBufferDispatch::GnmCommandBufferDispatch(vlt::VltDevice* device) :
		GnmCommandBuffer(device)
	{
		m_initializer = std::make_unique<GnmInitializer>(m_device, VltQueueType::Compute);
		m_context     = m_device->createContext();
	}

	GnmCommandBufferDispatch::~GnmCommandBufferDispatch()
	{
	}

	void GnmCommandBufferDispatch::initializeDefaultHardwareState()
	{
		// This the first packed of a frame.
		// We do some initialize work here.

		GnmCommandBuffer::initializeDefaultHardwareState();

		m_context->beginRecording(
			m_device->createCommandList(VltQueueType::Compute));
	}

	void GnmCommandBufferDispatch::setViewportTransformControl(ViewportTransformControl vportControl)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setPrimitiveSetup(PrimitiveSetup reg)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setScreenScissor(int32_t left, int32_t top, int32_t right, int32_t bottom)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setViewport(uint32_t viewportId, float dmin, float dmax, const float scale[3], const float offset[3])
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setHardwareScreenOffset(uint32_t offsetX, uint32_t offsetY)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setGuardBands(float horzClip, float vertClip, float horzDiscard, float vertDiscard)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setPsShaderUsage(const uint32_t* inputTable, uint32_t numItems)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setActiveShaderStages(ActiveShaderStages activeStages)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setPsShader(const gcn::PsStageRegisters* psRegs)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::updatePsShader(const gcn::PsStageRegisters* psRegs)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setVsShader(const gcn::VsStageRegisters* vsRegs, uint32_t shaderModifier)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setEmbeddedVsShader(EmbeddedVsShader shaderId, uint32_t shaderModifier)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::updateVsShader(const gcn::VsStageRegisters* vsRegs, uint32_t shaderModifier)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setVsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Buffer* buffer)
	{
		std::memcpy(&m_state.shaderContext.userData[startUserDataSlot], buffer, sizeof(Buffer));
	}

	void GnmCommandBufferDispatch::setTsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Texture* tex)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setSsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Sampler* sampler)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setPointerInUserData(ShaderStage stage, uint32_t startUserDataSlot, void* gpuAddr)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setUserDataRegion(ShaderStage stage, uint32_t startUserDataSlot, const uint32_t* userData, uint32_t numDwords)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setRenderTarget(uint32_t rtSlot, RenderTarget const* target)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDepthRenderTarget(DepthRenderTarget const* depthTarget)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDepthClearValue(float clearValue)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setStencilClearValue(uint8_t clearValue)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setRenderTargetMask(uint32_t mask)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setBlendControl(uint32_t rtSlot, BlendControl blendControl)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDepthStencilControl(DepthStencilControl depthControl)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDbRenderControl(DbRenderControl reg)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setVgtControl(uint8_t primGroupSizeMinusOne)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setPrimitiveType(PrimitiveType primType)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setIndexSize(IndexSize indexSize, CachePolicy cachePolicy)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::drawIndexAuto(uint32_t indexCount, DrawModifier modifier)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::drawIndexAuto(uint32_t indexCount)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::drawIndex(uint32_t indexCount, const void* indexAddr, DrawModifier modifier)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::drawIndex(uint32_t indexCount, const void* indexAddr)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
	{
		commitComputeState();

		m_context->dispatch(threadGroupX, threadGroupY, threadGroupZ);
	}

	void GnmCommandBufferDispatch::dispatchWithOrderedAppend(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ, DispatchOrderedAppendMode orderedAppendMode)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::writeDataInline(void* dstGpuAddr, const void* data, uint32_t sizeInDwords, WriteDataConfirmMode writeConfirm)
	{
		GnmCommandBuffer::writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
	}

	void GnmCommandBufferDispatch::writeDataInlineThroughL2(void* dstGpuAddr, const void* data, uint32_t sizeInDwords, CachePolicy cachePolicy, WriteDataConfirmMode writeConfirm)
	{
		GnmCommandBuffer::writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
	}

	void GnmCommandBufferDispatch::writeAtEndOfPipe(EndOfPipeEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy cachePolicy)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::writeAtEndOfPipeWithInterrupt(EndOfPipeEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy cachePolicy)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::writeAtEndOfShader(EndOfShaderEventType eventType, void* dstGpuAddr, uint32_t immValue)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::waitOnAddress(void* gpuAddr, uint32_t mask, WaitCompareFunc compareFunc, uint32_t refValue)
	{
		auto label = m_labelManager->getLabel(gpuAddr);
		label->wait(m_context.ptr(), mask, compareFunc, refValue);
	}

	void GnmCommandBufferDispatch::waitOnAddressAndStallCommandBufferParser(void* gpuAddr, uint32_t mask, uint32_t refValue)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::waitForGraphicsWrites(uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, CacheAction cacheAction, uint32_t extendedCacheMask, StallCommandBufferParserMode commandBufferStallMode)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDepthStencilDisable()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::flushShaderCachesAndWait(CacheAction cacheAction, uint32_t extendedCacheMask, StallCommandBufferParserMode commandBufferStallMode)
	{
	}

	void GnmCommandBufferDispatch::waitUntilSafeForRendering(uint32_t videoOutHandle, uint32_t displayBufferIndex)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::prepareFlip()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::prepareFlip(void* labelAddr, uint32_t value)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::prepareFlipWithEopInterrupt(EndOfPipeEventType eventType, CacheAction cacheAction)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::prepareFlipWithEopInterrupt(EndOfPipeEventType eventType, void* labelAddr, uint32_t value, CacheAction cacheAction)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setCsShader(const gcn::CsStageRegisters* computeData, uint32_t shaderModifier)
	{
		GnmCommandBuffer::setCsShader(m_state.shaderContext, computeData, shaderModifier);
	}

	void GnmCommandBufferDispatch::writeReleaseMemEventWithInterrupt(ReleaseMemEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy writePolicy)
	{
		VkPipelineStageFlags2 stage = eventType == kReleaseMemEventCsDone
										  ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
										  : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		auto label = m_labelManager->getLabel(dstGpuAddr);
		label->writeWithInterrupt(m_context.ptr(), stage, srcSelector, immValue);
	}

	void GnmCommandBufferDispatch::writeReleaseMemEvent(ReleaseMemEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy writePolicy)
	{
		VkPipelineStageFlags2 stage = eventType == kReleaseMemEventCsDone
										  ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
										  : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		auto label = m_labelManager->getLabel(dstGpuAddr);
		label->write(m_context.ptr(), stage, srcSelector, immValue);
	}

	void GnmCommandBufferDispatch::setVgtControlForNeo(uint8_t primGroupSizeMinusOne, WdSwitchOnlyOnEopMode wdSwitchOnlyOnEopMode, VgtPartialVsWaveMode partialVsWaveMode)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::commitComputeState()
	{
		GnmCommandBuffer::commitComputeState(m_state.shaderContext);

		m_initializer->flush();
	}

	void GnmCommandBufferDispatch::updateMetaBufferInfo(
		VkPipelineStageFlags stage, uint32_t startRegister, const Buffer* vsharp)
	{
		GcnBufferMeta meta = populateBufferMeta(vsharp);
		m_state.shaderContext.meta.cs.bufferInfos[startRegister] = meta;
	}

	void GnmCommandBufferDispatch::updateMetaTextureInfo(
		VkPipelineStageFlags stage, uint32_t startRegister, bool isDepth, const Texture* tsharp)
	{
		GcnTextureMeta meta = populateTextureMeta(tsharp, isDepth);
		m_state.shaderContext.meta.cs.textureInfos[startRegister] = meta;
	}

	void GnmCommandBufferDispatch::setClipControl(ClipControl reg)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setDbCountControl(DbCountControlPerfectZPassCounts perfectZPassCounts, uint32_t log2SampleRate)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setBorderColorTableAddr(void* tableAddr)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void* GnmCommandBufferDispatch::allocateFromCommandBuffer(uint32_t sizeInBytes, EmbeddedDataAlignment alignment)
	{
		return nullptr;
	}

	void GnmCommandBufferDispatch::setStencilSeparate(StencilControl front, StencilControl back)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setCbControl(CbMode mode, RasterOp op)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setStencilOpControl(StencilOpControl stencilControl)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::triggerEvent(EventType eventType)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::prefetchIntoL2(void* dataAddr, uint32_t sizeInBytes)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::setStencil(StencilControl stencilControl)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::pushMarker(const char* debugString)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::pushMarker(const char* debugString, uint32_t argbColor)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDispatch::popMarker()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

}  // namespace sce::Gnm