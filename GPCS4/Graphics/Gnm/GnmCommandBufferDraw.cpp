#include "GnmCommandBufferDraw.h"

#include "GnmBuffer.h"
#include "GnmConverter.h"
#include "GnmSampler.h"
#include "GnmSharpBuffer.h"
#include "GnmTexture.h"
#include "GnmGpuLabel.h"
#include "GpuAddress/GnmGpuAddress.h"

#include "Gcn/GcnUtil.h"
#include "Platform/PlatFile.h"
#include "Sce/SceGpuQueue.h"
#include "Sce/SceResourceTracker.h"
#include "Sce/SceLabelManager.h"
#include "Sce/SceVideoOut.h"
#include "Violet/VltContext.h"
#include "Violet/VltDevice.h"
#include "Violet/VltImage.h"
#include "Violet/VltRenderTarget.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>

LOG_CHANNEL(Graphic.Gnm.GnmCommandBufferDraw);

using namespace sce::vlt;
using namespace sce::gcn;

namespace sce::Gnm
{

// Use this to break on a shader you want to debug.
#define SHADER_DEBUG_BREAK(mod, token) \
	if (mod.name() == token)           \
	{                                  \
		__debugbreak();                \
	}


	GnmCommandBufferDraw::GnmCommandBufferDraw(vlt::VltDevice* device) :
		GnmCommandBuffer(device)
	{
		m_initializer = std::make_unique<GnmInitializer>(m_device, VltQueueType::Graphics);
		m_context     = m_device->createContext();
	}

	GnmCommandBufferDraw::~GnmCommandBufferDraw()
	{
	}

	void GnmCommandBufferDraw::initializeDefaultHardwareState()
	{
		// This the first packed of a frame.
		// We do some initialize work here.
		GnmCommandBuffer::initializeDefaultHardwareState();

		m_context->beginRecording(
			m_device->createCommandList(VltQueueType::Graphics));
	}

	void GnmCommandBufferDraw::setViewportTransformControl(ViewportTransformControl vportControl)
	{
		// TODO:
	}

	void GnmCommandBufferDraw::setPrimitiveSetup(PrimitiveSetup reg)
	{
		VkFrontFace     frontFace = reg.getFrontFace() == kPrimitiveSetupFrontFaceCcw ? 
			VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		VkPolygonMode   polyMode  = cvt::convertPolygonMode(reg.getPolygonModeFront());
		VkCullModeFlags cullMode  = cvt::convertCullMode(reg.getCullFace());

		VltRasterizerState rs = {
			polyMode,
			cullMode,
			frontFace,
			VK_FALSE,
			VK_FALSE,
			VK_SAMPLE_COUNT_1_BIT,
			VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT
		};

		m_context->setRasterizerState(rs);
	}

	void GnmCommandBufferDraw::setScreenScissor(int32_t left, int32_t top, int32_t right, int32_t bottom)
	{
		VkRect2D scissor;
		scissor.offset.x      = left;
		scissor.offset.y      = top;
		scissor.extent.width  = right - left;
		scissor.extent.height = bottom - top;
		m_context->setScissors(1, &scissor);
	}

	void GnmCommandBufferDraw::setViewport(uint32_t viewportId, float dmin, float dmax, const float scale[3], const float offset[3])
	{
		// The viewport's origin in Gnm is in the lower left of the screen,
		// with Y pointing up.
		// In Vulkan the origin is in the top left of the screen,
		// with Y pointing downwards.
		// We need to flip the viewport of gnm to adapt to vulkan.
		//
		// Note, this is going to work with VK_KHR_Maintenance1 extension enabled,
		// which is the default of Vulkan 1.1.
		// And we must use dynamic viewport state (vkCmdSetViewport), or negative viewport height won't work.

		float width  = scale[0] / 0.5f;
		float height = -scale[1] / 0.5f;
		float left   = offset[0] - scale[0];
		float top    = offset[1] + scale[1];

		VkViewport viewport;
		viewport.x        = left;
		viewport.y        = top + height;
		viewport.width    = width;
		viewport.height   = -height;
		viewport.minDepth = dmin;
		viewport.maxDepth = dmax;

		m_context->setViewports(1, &viewport);
	}

	void GnmCommandBufferDraw::setHardwareScreenOffset(uint32_t offsetX, uint32_t offsetY)
	{
		// TODO:
	}

	void GnmCommandBufferDraw::setGuardBands(float horzClip, float vertClip, float horzDiscard, float vertDiscard)
	{
		// TODO:
	}

	void GnmCommandBufferDraw::setPsShaderUsage(const uint32_t* inputTable, uint32_t numItems)
	{
		auto& ctx = m_state.shaderContext[kShaderStagePs];
		std::transform(inputTable, inputTable + numItems,
					   ctx.meta.ps.semanticMapping.begin(),
					   [](const uint32_t reg) {
						   return *reinterpret_cast<const PixelSemanticMapping*>(&reg);
					   });
		ctx.meta.ps.inputSemanticCount = numItems;
	}

	void GnmCommandBufferDraw::setActiveShaderStages(ActiveShaderStages activeStages)
	{
		// TODO:
	}

	void GnmCommandBufferDraw::setPsShader(const gcn::PsStageRegisters* psRegs)
	{
		auto& ctx = m_state.shaderContext[kShaderStagePs];
		ctx.code  = psRegs->getCodeAddress();

		const SPI_SHADER_PGM_RSRC2_PS* rsrc2 = reinterpret_cast<const SPI_SHADER_PGM_RSRC2_PS*>(&psRegs->spiShaderPgmRsrc2Ps);
		ctx.meta.ps.userSgprCount            = rsrc2->user_sgpr;

		const SPI_PS_INPUT_ENA* addr = reinterpret_cast<const SPI_PS_INPUT_ENA*>(&psRegs->spiPsInputAddr);
		ctx.meta.ps.perspSampleEn    = addr->persp_sample_ena;
		ctx.meta.ps.perspCenterEn    = addr->persp_center_ena;
		ctx.meta.ps.perspCentroidEn  = addr->persp_centroid_ena;
		ctx.meta.ps.perspPullModelEn = addr->persp_pull_model_ena;
		ctx.meta.ps.linearSampleEn   = addr->linear_sample_ena;
		ctx.meta.ps.linearCenterEn   = addr->linear_center_ena;
		ctx.meta.ps.linearCentroidEn = addr->linear_centroid_ena;
		ctx.meta.ps.posXEn           = addr->pos_x_float_ena;
		ctx.meta.ps.posYEn           = addr->pos_y_float_ena;
		ctx.meta.ps.posZEn           = addr->pos_z_float_ena;
		ctx.meta.ps.posWEn           = addr->pos_w_float_ena;
	}

	void GnmCommandBufferDraw::updatePsShader(const gcn::PsStageRegisters* psRegs)
	{
		LOG_ASSERT(false, "TODO");
	}

	void GnmCommandBufferDraw::setVsShader(const gcn::VsStageRegisters* vsRegs, uint32_t shaderModifier)
	{
		auto& ctx = m_state.shaderContext[kShaderStageVs];
		ctx.code  = vsRegs->getCodeAddress();

		const SPI_SHADER_PGM_RSRC2_VS* rsrc2 = reinterpret_cast<const SPI_SHADER_PGM_RSRC2_VS*>(&vsRegs->spiShaderPgmRsrc2Vs);
		ctx.meta.vs.userSgprCount = rsrc2->user_sgpr;
	}

	void GnmCommandBufferDraw::setEmbeddedVsShader(EmbeddedVsShader shaderId, uint32_t shaderModifier)
	{
		LOG_ASSERT(shaderId == kEmbeddedVsShaderFullScreen, "invalid shader id %d", shaderId);

		// const static uint8_t embeddedVsShaderFullScreen[] = {
		//	0xFF, 0x03, 0xEB, 0xBE, 0x07, 0x00, 0x00, 0x00, 0x81, 0x00, 0x02, 0x36, 0x81, 0x02, 0x02, 0x34,
		//	0xC2, 0x00, 0x00, 0x36, 0xC1, 0x02, 0x02, 0x4A, 0xC1, 0x00, 0x00, 0x4A, 0x01, 0x0B, 0x02, 0x7E,
		//	0x00, 0x0B, 0x00, 0x7E, 0x80, 0x02, 0x04, 0x7E, 0xF2, 0x02, 0x06, 0x7E, 0xCF, 0x08, 0x00, 0xF8,
		//	0x01, 0x00, 0x02, 0x03, 0x0F, 0x02, 0x00, 0xF8, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x81, 0xBF,
		//	0x4F, 0x72, 0x62, 0x53, 0x68, 0x64, 0x72, 0x07, 0x47, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		//	0x9F, 0xC2, 0xF8, 0x47, 0xCF, 0xA5, 0x2D, 0x9B, 0x7D, 0x5B, 0x7C, 0xFF, 0x17, 0x00, 0x00, 0x00
		// };

		// Above is the original Gnm embedded vs shader for kEmbeddedVsShaderFullScreen.
		// It outputs vertex:
		// 0  (-1.0, -1.0, 0.0, 1.0)
		// 1  (1.0, -1.0, 0.0, 1.0)
		// 2  (-1.0, 1.0, 0.0, 1.0)
		// And treated it as a rectangle list,
		// this will only cover the bottom-left triangle
		// of the screen, since vulkan doesn't
		// support rect list vertex format, so we
		// have to use triangle list.

		// Below is our replaced version.
		// It outputs vertex:
		// 0  (-1.0, -1.0, 0.0, 1.0)
		// 1  (-1.0, 3.0, 0.0, 1.0)
		// 2  (3.0, -1.0, 0.0, 1.0)
		// We treated it as triangle list,
		// and this way we cover the whole screen.

		// Note:
		// The generated vertex data is in clockwise,
		// thus we must make sure the front face is
		// VK_FRONT_FACE_CLOCKWISE. And if culling is enabled,
		// it must be VK_CULL_MODE_BACK_BIT.

		// Source code
		/*
		struct VS_OUTPUT
		{
			float4 vPosition  :  S_POSITION;
			float2 vTexcoord  :  TEXCOORD0;
		};

		VS_OUTPUT main(uint VertexId:S_VERTEX_ID)
		{
			VS_OUTPUT Output;

			Output.vTexcoord = float2(
			float(VertexId & 2),
			float(VertexId & 1) * 2.0);

			Output.vPosition = float4(-1.0 + 2.0 * Output.vTexcoord, 0.0, 1.0);
			return Output;
		}
		*/

		const static uint8_t embeddedVsShaderFullScreen[] = {
			0xFF, 0x03, 0xEB, 0xBE, 0x09, 0x00, 0x00, 0x00, 0x81, 0x00, 0x02, 0x36, 0x82, 0x00, 0x00, 0x36,
			0x00, 0x0D, 0x00, 0x7E, 0x01, 0x0D, 0x04, 0x7E, 0x03, 0x00, 0x82, 0xD2, 0xF4, 0x00, 0xCE, 0x03,
			0x04, 0x00, 0x82, 0xD2, 0xF6, 0x04, 0xCE, 0x03, 0x80, 0x02, 0x02, 0x7E, 0xF2, 0x02, 0x0A, 0x7E,
			0xCF, 0x08, 0x00, 0xF8, 0x03, 0x04, 0x01, 0x05, 0xF4, 0x04, 0x04, 0x10, 0x0F, 0x02, 0x00, 0xF8,
			0x00, 0x02, 0x01, 0x01, 0x00, 0x00, 0x81, 0xBF, 0x02, 0x03, 0x00, 0x00, 0x1C, 0x61, 0x6D, 0x04,
			0x4F, 0x72, 0x62, 0x53, 0x68, 0x64, 0x72, 0x07, 0x45, 0x48, 0x00, 0x00, 0x02, 0x00, 0x08, 0x05,
			0x61, 0xDE, 0xE7, 0xD1, 0x00, 0x00, 0x00, 0x00, 0x98, 0xE5, 0xCA, 0xB9
		};

		auto& ctx                 = m_state.shaderContext[kShaderStageVs];
		ctx.code                  = reinterpret_cast<const void*>(embeddedVsShaderFullScreen);
		ctx.meta.vs.userSgprCount = 0;
	}

	void GnmCommandBufferDraw::updateVsShader(const gcn::VsStageRegisters* vsRegs, uint32_t shaderModifier)
	{
	}

	void GnmCommandBufferDraw::setVsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Buffer* buffer)
	{
		std::memcpy(&m_state.shaderContext[stage].userData[startUserDataSlot], buffer, sizeof(Buffer));
	}

	void GnmCommandBufferDraw::setTsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Texture* tex)
	{
		std::memcpy(&m_state.shaderContext[stage].userData[startUserDataSlot], tex, sizeof(Texture));
	}

	void GnmCommandBufferDraw::setSsharpInUserData(ShaderStage stage, uint32_t startUserDataSlot, const Sampler* sampler)
	{
		std::memcpy(&m_state.shaderContext[stage].userData[startUserDataSlot], sampler, sizeof(Sampler));
	}

	void GnmCommandBufferDraw::setPointerInUserData(ShaderStage stage, uint32_t startUserDataSlot, void* gpuAddr)
	{
		std::memcpy(&m_state.shaderContext[stage].userData[startUserDataSlot], gpuAddr, sizeof(void*));
	}

	void GnmCommandBufferDraw::setUserDataRegion(ShaderStage stage, uint32_t startUserDataSlot, const uint32_t* userData, uint32_t numDwords)
	{
		std::memcpy(&m_state.shaderContext[stage].userData[startUserDataSlot], userData, numDwords * sizeof(uint32_t));
	}

	void GnmCommandBufferDraw::setRenderTarget(uint32_t rtSlot, RenderTarget const* target)
	{
		auto resource = m_tracker->find(target->getBaseAddress());
		do
		{
			Rc<VltImageView> targetView = nullptr;
			if (!resource)
			{
				// The render target is not a display buffer registered in video out,
				// we create a new one.
				SceRenderTarget rtRes;
				m_factory.createRenderTarget(target, rtRes);

				Texture rtTexture;
				rtTexture.initFromRenderTarget(target, false);
				m_initializer->initTexture(rtRes.image, &rtTexture);

				m_tracker->track(rtRes);
			}
			else
			{
				// Record display buffer.
				m_state.om.displayRenderTarget = resource;

				// update render target
				SceRenderTarget rtRes = {};
				rtRes.image           = resource->renderTarget().image;
				rtRes.imageView       = resource->renderTarget().imageView;
				// replace the dummy target with real one
				rtRes.renderTarget = *target;
				resource->setRenderTarget(rtRes);

				targetView = rtRes.imageView;
			}

			VltAttachment attachment = 
			{
				targetView,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
			m_context->bindRenderTarget(rtSlot, attachment);

		} while (false);
	}

	void GnmCommandBufferDraw::setDepthRenderTarget(DepthRenderTarget const* depthTarget)
	{
		do
		{
			if (depthTarget == nullptr)
			{
				m_context->bindDepthRenderTarget(VltAttachment{});
				break;
			}

			auto zBufferAddr = depthTarget->getZReadAddress();
			auto resource    = m_tracker->find(zBufferAddr);

			Rc<VltImageView> depthView = nullptr;
			if (!resource)
			{
				// create a new depth image and track it
				SceDepthRenderTarget depthResource = {};
				m_factory.createDepthImage(depthTarget, depthResource);
				depthView = depthResource.imageView;

				auto iter = m_tracker->track(depthResource).first;
				resource  = &iter->second;
			}
			else
			{
				auto type = resource->type();
				if (!type.test(SceResourceType::DepthRenderTarget))
				{
					SceDepthRenderTarget depthResource = {};
					m_factory.createDepthImage(depthTarget, depthResource);
					depthView = depthResource.imageView;

					resource->setDepthRenderTarget(depthResource);
					// Pending upload
					resource->setTransform(SceTransformFlag::GpuUpload);
				}
				else
				{
					depthView = resource->depthRenderTarget().imageView;
				}
			}

			VltAttachment attachment = {
				depthView,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
			};
			m_context->bindDepthRenderTarget(attachment);

		} while (false);
	}

	void GnmCommandBufferDraw::setDepthClearValue(float clearValue)
	{
		VkClearValue value;
		value.depthStencil.depth = clearValue;
		m_context->setDepthClearValue(value);
	}

	void GnmCommandBufferDraw::setStencilClearValue(uint8_t clearValue)
	{
		VkClearValue value;
		value.depthStencil.stencil = clearValue;
		m_context->setStencilClearValue(value);
	}

	void GnmCommandBufferDraw::setRenderTargetMask(uint32_t mask)
	{
		auto writeMasks = cvt::convertRenderTargetMask(mask);
		for (uint32_t attachment = 0; attachment != writeMasks.size(); ++attachment)
		{
			m_context->setBlendMask(
				attachment, writeMasks[attachment]);
		}
	}

	void GnmCommandBufferDraw::setBlendControl(uint32_t rtSlot, BlendControl blendControl)
	{
		VkBlendFactor colorSrcFactor = cvt::convertBlendMultiplier(blendControl.getColorEquationSourceMultiplier());
		VkBlendFactor colorDstFactor = cvt::convertBlendMultiplier(blendControl.getColorEquationDestinationMultiplier());
		VkBlendOp     colorBlendOp   = cvt::convertBlendFunc(blendControl.getColorEquationBlendFunction());

		VkBlendFactor alphaSrcFactor = cvt::convertBlendMultiplier(blendControl.getAlphaEquationSourceMultiplier());
		VkBlendFactor alphaDstFactor = cvt::convertBlendMultiplier(blendControl.getAlphaEquationDestinationMultiplier());
		VkBlendOp     alphaBlendOp   = cvt::convertBlendFunc(blendControl.getAlphaEquationBlendFunction());

		VkColorComponentFlags fullMask = 
			VK_COLOR_COMPONENT_R_BIT | 
			VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | 
			VK_COLOR_COMPONENT_A_BIT;  

		// Here we set color write mask to fullMask.
		// The real mask value should be set through setRenderTargetMask call.

		VltBlendMode blend = {
			(VkBool32)blendControl.getBlendEnable(),
			colorSrcFactor,
			colorDstFactor,
			colorBlendOp,
			alphaSrcFactor,
			alphaDstFactor,
			alphaBlendOp,
			fullMask
		};

		m_context->setBlendMode(rtSlot, blend);

		VltLogicOpState loState;
		loState.enableLogicOp = VK_FALSE;
		loState.logicOp       = VK_LOGIC_OP_NO_OP;

		m_context->setLogicOpState(loState);
	}

	void GnmCommandBufferDraw::setDepthStencilControl(DepthStencilControl depthControl)
	{
		LOG_ASSERT(depthControl.stencilEnable == false, "stencil test not supported yet.");

		VkCompareOp depthCmpOp = cvt::convertCompareFunc(depthControl.getDepthControlZCompareFunction());
		VkCompareOp stencilFront = cvt::convertCompareFunc(depthControl.getStencilFunction());
		VkCompareOp stencilBack = cvt::convertCompareFunc(depthControl.getStencilFunctionBack());

		VkStencilOpState frontOp = {};
		frontOp.compareOp        = stencilFront;
		VkStencilOpState backOp  = {};
		backOp.compareOp         = stencilBack;

		VltDepthStencilState ds = {
			(VkBool32)depthControl.depthEnable,
			(VkBool32)depthControl.zWrite,
			(VkBool32)depthControl.stencilEnable,
			depthCmpOp,
			frontOp,
			backOp
		};

		// We use depth bounds test to emulate DbRenderControl
		m_context->setDepthBoundsTestEnable(m_state.ds.dbClearDepth
												? VK_TRUE
												: depthControl.depthBoundsEnable);

		m_context->setDepthStencilState(ds);
	}

	void GnmCommandBufferDraw::setDbRenderControl(DbRenderControl reg)
	{
		bool depthClear    = reg.getDepthClearEnable();
		bool htielCompress = reg.getHtileResummarizeEnable();
		if (depthClear && !htielCompress)
		{
			// In Gnm, when depth clear enable and HTILE compress disable
			// all writes to the depth buffer will use the depth clear value set by
			// DrawCommandBuffer::setDepthClearValue() instead of the fragment's depth value.
			// 
			// For vulkan, we use depth bound test to emulate this somehow.
			// We first set the depth clear value to clear depth buffer once render pass begin.
			// Then force depth bound test failed to leave depth buffer untouched.
			// This way the depth buffer remains the clear value.

			// TODO:
			// This approach is not accurate, fix it in the future.

			m_context->setDepthBoundsTestEnable(VK_TRUE);

			VltDepthBoundsRange depthBounds;
			depthBounds.minDepthBounds    = 1.0;
			depthBounds.maxDepthBounds    = 0.0;
			m_context->setDepthBoundsRange(depthBounds);

			m_state.ds.dbClearDepth = true;
		}
		else
		{
			m_context->setDepthBoundsTestEnable(VK_FALSE);
			m_state.ds.dbClearDepth = false;
		}

		
	}

	void GnmCommandBufferDraw::setVgtControl(uint8_t primGroupSizeMinusOne)
	{
	}

	void GnmCommandBufferDraw::setPrimitiveType(PrimitiveType primType)
	{
		VkPrimitiveTopology topology = cvt::convertPrimitiveType(primType);
		
		// TODO:
		// This is a temporary solution, mainly for embedded vertex shader.
		// For a primitive type which is not supported by vulkan natively,
		// we need to find a workaround.
		if (primType == kPrimitiveTypeRectList)
		{
			topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}

		LOG_ASSERT(topology != VK_PRIMITIVE_TOPOLOGY_MAX_ENUM, "primType not supported.");
		m_state.ia.topology = topology;

		VltInputAssemblyState ia = {
			topology,
			VK_FALSE,
			0
		};
		m_context->setInputAssemblyState(ia);
	}

	void GnmCommandBufferDraw::setIndexSize(IndexSize indexSize, CachePolicy cachePolicy)
	{
		m_state.ia.indexType = cvt::convertIndexSize(indexSize);
	}

	void GnmCommandBufferDraw::drawIndexAuto(uint32_t indexCount, DrawModifier modifier)
	{
		// If the index size is currently 32 bits, this command will partially set it to 16 bits
		m_state.ia.indexType   = VK_INDEX_TYPE_UINT16;
		m_state.ia.indexBuffer = generateIndexBufferAuto(indexCount);

		commitGraphicsState();

		m_context->drawIndexed(indexCount, 1, 0, 0, 0);
	}

	void GnmCommandBufferDraw::drawIndexAuto(uint32_t indexCount)
	{
		DrawModifier modifier;
		modifier.renderTargetSliceOffset = 0;
		drawIndexAuto(indexCount, modifier);
	}

	void GnmCommandBufferDraw::drawIndex(uint32_t indexCount, const void* indexAddr, DrawModifier modifier)
	{
		uint32_t indexBufferSize =
			m_state.ia.indexType == VK_INDEX_TYPE_UINT16
				? sizeof(uint16_t) * indexCount
				: sizeof(uint32_t) * indexCount;

		m_state.ia.indexBuffer = generateIndexBuffer(indexAddr, indexBufferSize);

		commitGraphicsState();

		m_context->drawIndexed(indexCount, 1, 0, 0, 0);
	}

	void GnmCommandBufferDraw::drawIndex(uint32_t indexCount, const void* indexAddr)
	{
		DrawModifier modifier;
		modifier.renderTargetSliceOffset = 0;
		drawIndex(indexCount, indexAddr, modifier);
	}

	void GnmCommandBufferDraw::dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
	{
		commitComputeState();

		m_context->dispatch(threadGroupX, threadGroupY, threadGroupZ);
	}

	void GnmCommandBufferDraw::dispatchWithOrderedAppend(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ, DispatchOrderedAppendMode orderedAppendMode)
	{
	}

	void GnmCommandBufferDraw::writeDataInline(void* dstGpuAddr, const void* data, uint32_t sizeInDwords, WriteDataConfirmMode writeConfirm)
	{
		GnmCommandBuffer::writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
	}

	void GnmCommandBufferDraw::writeDataInlineThroughL2(void* dstGpuAddr, const void* data, uint32_t sizeInDwords, CachePolicy cachePolicy, WriteDataConfirmMode writeConfirm)
	{
		GnmCommandBuffer::writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
	}

	void GnmCommandBufferDraw::writeAtEndOfPipe(EndOfPipeEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy cachePolicy)
	{
		VkPipelineStageFlags2 stage = eventType == kEopCsDone
										  ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
										  : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		auto label = m_labelManager->getLabel(dstGpuAddr);
		label->write(m_context.ptr(), stage, srcSelector, immValue);
	}

	void GnmCommandBufferDraw::writeAtEndOfPipeWithInterrupt(EndOfPipeEventType eventType, EventWriteDest dstSelector, void* dstGpuAddr, EventWriteSource srcSelector, uint64_t immValue, CacheAction cacheAction, CachePolicy cachePolicy)
	{
		VkPipelineStageFlags2 stage = eventType == kEopCsDone
										  ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
										  : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		auto label = m_labelManager->getLabel(dstGpuAddr);
		label->writeWithInterrupt(m_context.ptr(), stage, srcSelector, immValue);
	}

	void GnmCommandBufferDraw::writeAtEndOfShader(EndOfShaderEventType eventType, void* dstGpuAddr, uint32_t immValue)
	{
		VkPipelineStageFlags2 stage = eventType == kEosPsDone
										  ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
										  : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		auto label = m_labelManager->getLabel(dstGpuAddr);
		label->write(m_context.ptr(), stage, kEventWriteSource64BitsImmediate, immValue);
	}

	void GnmCommandBufferDraw::waitOnAddress(void* gpuAddr, uint32_t mask, WaitCompareFunc compareFunc, uint32_t refValue)
	{
		auto label = m_labelManager->getLabel(gpuAddr);
		label->wait(m_context.ptr(), mask, compareFunc, refValue);
	}

	void GnmCommandBufferDraw::waitOnAddressAndStallCommandBufferParser(void* gpuAddr, uint32_t mask, uint32_t refValue)
	{
		LOG_ASSERT(false, "TODO");
	}

	void GnmCommandBufferDraw::waitForGraphicsWrites(uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, CacheAction cacheAction, uint32_t extendedCacheMask, StallCommandBufferParserMode commandBufferStallMode)
	{
		// TODO:
		// This should be done more accurately,
		// e.g. specify the render target image and use an image barrier.
		m_context->emitRenderTargetReadbackBarrier();
	}

	void GnmCommandBufferDraw::setDepthStencilDisable()
	{
		VltDepthStencilState ds = {};
		m_context->setDepthStencilState(ds);
	}

	void GnmCommandBufferDraw::setClipControl(ClipControl reg)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::flushShaderCachesAndWait(CacheAction cacheAction, uint32_t extendedCacheMask, StallCommandBufferParserMode commandBufferStallMode)
	{
	}

	void GnmCommandBufferDraw::waitUntilSafeForRendering(uint32_t videoOutHandle, uint32_t displayBufferIndex)
	{
		// This cmd blocks command processor until the specified display buffer is no longer displayed.
		// should we call vkAcquireNextImageKHR here to implement it?
		// or should we create a new render target image and then bilt to swapchain like DXVK does?

		// get render target from swapchain
		auto& tracker    = GPU().resourceTracker();
		auto& videoOut   = GPU().videoOutGet(videoOutHandle);
		auto  dispBuffer = videoOut.getDisplayBuffer(displayBufferIndex);

		auto res = tracker.find(dispBuffer.address);
		if (res)
		{
			auto& image = res->renderTarget().image;
			auto  range = res->renderTarget().imageView->imageSubresources();
			m_context->transformImage(
				image,
				range,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		}
	}

	void GnmCommandBufferDraw::prepareFlip()
	{
		onPrepareFlip();
	}

	void GnmCommandBufferDraw::prepareFlip(void* labelAddr, uint32_t value)
	{
		*(uint32_t*)labelAddr = value;
		onPrepareFlip();
	}

	void GnmCommandBufferDraw::prepareFlipWithEopInterrupt(EndOfPipeEventType eventType, CacheAction cacheAction)
	{
		onPrepareFlip();
	}

	void GnmCommandBufferDraw::prepareFlipWithEopInterrupt(EndOfPipeEventType eventType, void* labelAddr, uint32_t value, CacheAction cacheAction)
	{
		*(uint32_t*)labelAddr = value;
		onPrepareFlip();
	}

	void GnmCommandBufferDraw::setCsShader(const gcn::CsStageRegisters* computeData, uint32_t shaderModifier)
	{
		auto& ctx = m_state.shaderContext[kShaderStageCs];
		GnmCommandBuffer::setCsShader(ctx, computeData, shaderModifier);
	}

	void GnmCommandBufferDraw::setVgtControlForNeo(uint8_t primGroupSizeMinusOne, WdSwitchOnlyOnEopMode wdSwitchOnlyOnEopMode, VgtPartialVsWaveMode partialVsWaveMode)
	{
	}

	Rc<VltBuffer> GnmCommandBufferDraw::generateIndexBuffer(const void* data, uint32_t size)
	{
		Buffer dummy = {};
		dummy.initAsDataBuffer(data, kDataFormatR16Uint, size / kDataFormatR16Uint.getBytesPerElement());

		GnmBufferCreateInfo info;
		info.vsharp     = &dummy;
		info.usage      = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.stage      = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access     = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		info.memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		SceBuffer buffer = getResourceBuffer(info);

		return buffer.buffer;
	}

	Rc<VltBuffer> GnmCommandBufferDraw::generateIndexBufferAuto(uint32_t indexCount)
	{
		// Auto-generated indexes are forced in 16 bits width.
		std::vector<uint16_t> indexes;

		switch (m_state.ia.topology)
		{
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		{
			indexes.resize(indexCount);
			std::generate(indexes.begin(), indexes.end(),
				[n = 0]() mutable -> uint16_t { return n++; });
		}
			break;
		default:
			LOG_ASSERT(false, "topology type not supported.");
			break;
		}

		return generateIndexBuffer(indexes.data(), sizeof(uint16_t) * indexes.size());
	}

	bool GnmCommandBufferDraw::isSingleVertexBinding(
		const uint32_t*                 vtxTable,
		const VertexInputSemanticTable& semanticTable)
	{
		struct VertexElement
		{
			void*    data;
			uint32_t stride;
		};

		std::array<VertexElement, kMaxVertexBufferCount> vtxData;

		uint32_t semanticCount = semanticTable.size();
		for (uint32_t i = 0; i != semanticCount; ++i)
		{
			auto&    sema           = semanticTable[i];
			uint32_t offsetInDwords = sema.m_semantic * ShaderConstantDwordSize::kDwordSizeVertexBuffer;
			const Buffer* vtxBuffer = reinterpret_cast<const Buffer*>(vtxTable + offsetInDwords);

			vtxData[i].data   = vtxBuffer->getBaseAddress();
			vtxData[i].stride = vtxBuffer->getStride();
		}

		void* firstVertextStart = vtxData[0].data;
		void* firstVertextEnd   = reinterpret_cast<uint8_t*>(firstVertextStart) + vtxData[0].stride;

		bool isSingleBinding = true;
		// If all left vertex attribute data start address is within the first and second
		// vertex address of the first attribute data, 
		// we think the game uses a single vertex buffer binding.
		// Otherwise we use multiple bindings.
		for (uint32_t i = 1; i != semanticCount; ++i)
		{
			void* vertex = vtxData[i].data;
			isSingleBinding &= (vertex > firstVertextStart && vertex < firstVertextEnd);
		}
		return isSingleBinding;
	}

	inline void GnmCommandBufferDraw::bindVertexBuffer(
		const Buffer* vsharp, uint32_t binding)
	{
		GnmBufferCreateInfo info;
		info.vsharp     = vsharp;
		info.usage      = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.stage      = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access     = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		info.memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		SceBuffer buffer = getResourceBuffer(info);

		m_context->bindVertexBuffer(binding,
									VltBufferSlice(buffer.buffer, 0, buffer.buffer->info().size),
									vsharp->getStride());
	}

	void GnmCommandBufferDraw::updateVertexBinding(GcnModule& vsModule)
	{
		auto& ctx      = m_state.shaderContext[kShaderStageVs];
		auto& resTable = vsModule.getResourceTable();

		// Find fetch shader
		VertexInputSemanticTable semaTable;
		auto fsCode = findFetchShader(resTable, ctx.userData);
		if (fsCode != nullptr)
		{
			GcnFetchShader fs(reinterpret_cast<const uint8_t*>(fsCode));
			semaTable = fs.getVertexInputSemanticTable();
		}

		// Update input layout
		if (!semaTable.empty())
		{
			int32_t vertexTableReg = findUsageRegister(resTable, kShaderInputUsagePtrVertexBufferTable);
			LOG_ASSERT(vertexTableReg >= 0, "vertex table not found while input semantic exist.");
			const uint32_t* vertexTable = *reinterpret_cast<uint32_t* const*>(&ctx.userData[vertexTableReg]);

			bool singleBinding = isSingleVertexBinding(vertexTable, semaTable);

			std::array<VltVertexAttribute, kMaxVertexBufferCount> attributes;
			std::array<VltVertexBinding, kMaxVertexBufferCount>   bindings;

			size_t   firstAttributeOffset = 0;
			uint32_t semanticCount        = semaTable.size();
			for (uint32_t i = 0; i != semanticCount; ++i)
			{
				auto&    sema           = semaTable[i];
				uint32_t offsetInDwords = sema.m_semantic * ShaderConstantDwordSize::kDwordSizeVertexBuffer;
				// We need to trust format info in V#, not instructions in fetch shader.
				// From GPU ISA:
				// The number of bytes loaded is determined solely by sV#.dfmt,
				// even if the instruction op count does not match.
				const Buffer* vsharp = reinterpret_cast<const Buffer*>(vertexTable + offsetInDwords);

				if (firstAttributeOffset == 0)
				{
					firstAttributeOffset = reinterpret_cast<size_t>(vsharp->getBaseAddress());
				}

				LOG_ASSERT(sema.m_semantic == i, "semantic index is not equal to table index.");

				// Attributes
				attributes[i].location = sema.m_semantic;
				attributes[i].binding  = singleBinding ? 
					0 : sema.m_semantic;
				attributes[i].format   = cvt::convertDataFormat(vsharp->getDataFormat());
				attributes[i].offset   = singleBinding ?
					reinterpret_cast<size_t>(vsharp->getBaseAddress()) - firstAttributeOffset : 0;

				// Bindings
				bindings[i].binding   = sema.m_semantic;
				bindings[i].fetchRate = 0;
				bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				// Fix element count
				sema.m_sizeInElements = 
					std::min(static_cast<uint32_t>(sema.m_sizeInElements), vsharp->getDataFormat().getNumComponents());
			}

			m_context->setInputLayout(
				semanticCount,
				attributes.data(),
				singleBinding ? 1 : semanticCount,
				bindings.data());

			// Create, upload and bind vertex buffer
			for (uint32_t i = 0; i != semanticCount; ++i)
			{
				auto&         sema           = semaTable[i];
				uint32_t      offsetInDwords = sema.m_semantic * ShaderConstantDwordSize::kDwordSizeVertexBuffer;
				const Buffer* vsharp         = reinterpret_cast<const Buffer*>(vertexTable + offsetInDwords);

				bindVertexBuffer(vsharp, sema.m_semantic);

				if (singleBinding)
				{
					break;
				}
			}

			// Record shader meta info
			ctx.meta.vs.inputSemanticCount = semanticCount;
			std::memcpy(
				ctx.meta.vs.inputSemanticTable.data(),
				semaTable.data(),
				sizeof(VertexInputSemantic) * semanticCount);
		}
		else
		{
			// No vertex buffer bind to the pipeline.
			m_context->setInputLayout(
				0, nullptr,
				0, nullptr);
		}
	}

	void GnmCommandBufferDraw::updateVertexShaderStage()
	{
		// Update vertex input
		auto& ctx = m_state.shaderContext[kShaderStageVs];

		do 
		{
			if (ctx.code == nullptr)
			{
				break;
			}

			// Update index
			// All draw calls in Gnm need index buffer.
			auto& indexBuffer = m_state.ia.indexBuffer;
			m_context->bindIndexBuffer(
				VltBufferSlice(indexBuffer, 0, indexBuffer->info().size),
				m_state.ia.indexType);

			GcnModule vsModule(
				GcnProgramType::VertexShader,
				reinterpret_cast<const uint8_t*>(ctx.code));

			auto& resTable = vsModule.getResourceTable();

			//// Update input layout and bind vertex buffer
			updateVertexBinding(vsModule);

			//// create and bind shader resources
			bindResource(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, resTable, ctx.userData);

			// bind the shader
			m_context->bindShader(
				VK_SHADER_STAGE_VERTEX_BIT,
				vsModule.compile(ctx.meta, m_moduleInfo));
		} while (false);
	}

	void GnmCommandBufferDraw::updatePixelShaderStage()
	{
		auto& ctx = m_state.shaderContext[kShaderStagePs];

		do 
		{
			if (ctx.code == nullptr)
			{
				break;
			}

			GcnModule psModule(
				GcnProgramType::PixelShader,
				reinterpret_cast<const uint8_t*>(ctx.code));

			auto& resTable = psModule.getResourceTable();

			// create and bind shader resources
			bindResource(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, resTable, ctx.userData);

			// bind the shader
			m_context->bindShader(
				VK_SHADER_STAGE_FRAGMENT_BIT,
				psModule.compile(ctx.meta, m_moduleInfo));
		} while (false);
	}

	void GnmCommandBufferDraw::commitGraphicsState()
	{
		updateVertexShaderStage();

		updatePixelShaderStage();

		// Set default ms state
		VltMultisampleState msState;
		msState.enableAlphaToCoverage = VK_FALSE;
		msState.sampleMask            = 0xFFFFFFFF;
		m_context->setMultisampleState(msState);

		// Flush memory to buffer and texture resources.
		m_initializer->flush();
		// Process pending upload/download
		m_tracker->transform(m_context.ptr());
	}

	void GnmCommandBufferDraw::commitComputeState()
	{
		auto& ctx = m_state.shaderContext[kShaderStageCs];

		GnmCommandBuffer::commitComputeState(ctx);

		m_initializer->flush();
	}

	const void* GnmCommandBufferDraw::findFetchShader(
		const gcn::GcnShaderResourceTable& table,
		const UserDataArray&               userData)
	{
		const void* fsCode = nullptr;

		int32_t fsReg = findUsageRegister(table, kShaderInputUsageSubPtrFetchShader);
		if (fsReg >= 0)
		{
			fsCode = *reinterpret_cast<void* const*>(&userData[fsReg]);
		}
		return fsCode;
	}

	void GnmCommandBufferDraw::onPrepareFlip()
	{
		// This is the last cmd for a command buffer submission,
		// we can do some finish works before submit and present.

	}

	void GnmCommandBufferDraw::updateMetaTextureInfo(
		VkPipelineStageFlags stage,
		uint32_t             startRegister,
		bool                 isDepth,
		const Texture*       tsharp)
	{
		// T# information is ripped upon uploading shader binary to GPU,
		// yet we need these information to proper declare image resource
		// when recompiling shaders.

		GcnTextureMeta meta = populateTextureMeta(tsharp, isDepth);

		auto  shaderStage = getShaderStage(stage);
		auto& ctx         = m_state.shaderContext[shaderStage];

		switch (stage)
		{
			case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
				ctx.meta.ps.textureInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
				ctx.meta.vs.textureInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:
				ctx.meta.cs.textureInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
				ctx.meta.gs.textureInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:
			case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:
			default:
				LOG_ASSERT(false, "TODO: stage %d is not supported yet, please support it.", stage);
				break;
		}
	}

	void GnmCommandBufferDraw::updateMetaBufferInfo(
		VkPipelineStageFlags stage,
		uint32_t             startRegister,
		const Buffer*        vsharp)
	{
		GcnBufferMeta meta = populateBufferMeta(vsharp);

		auto  shaderStage = getShaderStage(stage);
		auto& ctx         = m_state.shaderContext[shaderStage];

		switch (stage)
		{
			case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
				ctx.meta.ps.bufferInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
				ctx.meta.vs.bufferInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:
				ctx.meta.cs.bufferInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
				ctx.meta.gs.bufferInfos[startRegister] = meta;
				break;
			case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:
			case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:
			default:
				LOG_ASSERT(false, "TODO: stage %d is not supported yet, please support it.", stage);
				break;
		}
	}

	void GnmCommandBufferDraw::setDbCountControl(DbCountControlPerfectZPassCounts perfectZPassCounts, uint32_t log2SampleRate)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::setBorderColorTableAddr(void* tableAddr)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void* GnmCommandBufferDraw::allocateFromCommandBuffer(uint32_t sizeInBytes, EmbeddedDataAlignment alignment)
	{
		return nullptr;
	}

	void GnmCommandBufferDraw::setStencilSeparate(StencilControl front, StencilControl back)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::setCbControl(CbMode mode, RasterOp op)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::setStencilOpControl(StencilOpControl stencilControl)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::triggerEvent(EventType eventType)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::prefetchIntoL2(void* dataAddr, uint32_t sizeInBytes)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::setStencil(StencilControl stencilControl)
	{
		// throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::pushMarker(const char* debugString)
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::pushMarker(const char* debugString, uint32_t argbColor)
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}

	void GnmCommandBufferDraw::popMarker()
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}

}  // namespace sce::Gnm