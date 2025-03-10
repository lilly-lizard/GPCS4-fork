#include "GnmCommandProcessor.h"

#include "GnmBuffer.h"
#include "GnmGfx9MePm4Packets.h"
#include "GnmSampler.h"
#include "GnmTexture.h"
#include "UtilBit.h"

#include "Gcn/GcnShaderRegister.h"
#include "Violet/VltBuffer.h"

using namespace util;
using namespace sce::vlt;

LOG_CHANNEL(Graphic.Gnm.GnmCommandProcessor);

namespace sce::Gnm
{

	// *******
	// Important Note:
	// *******
	//
	// When parsing a PM4 packet, consider to cast PM4 packet to proper structure type
	// defined in GnmGfx9MePm4Packets.h first, if there's no proper definition,
	// see si_ci_vi_merged_pm4defs.h next.
	//
	// But also note, some structures are defined better in GnmGfx9MePm4Packets.h,
	// others are better in si_ci_vi_merged_pm4defs.h.
	// We'd better see both, and then choose the best version.
	//
	// If it's in si_ci_vi_merged_pm4defs.h, please copy it to GnmGfx9MePm4Packets.h and reformat code style.

	const uint32_t c_stageBases[kShaderStageCount] = { 0x2E40, 0x2C0C, 0x2C4C, 0x2C8C, 0x2CCC, 0x2D0C, 0x2D4C };

	GnmCommandProcessor::GnmCommandProcessor() :
		m_cb(nullptr)
	{
	}

	GnmCommandProcessor::~GnmCommandProcessor()
	{
	}

	void GnmCommandProcessor::attachCommandBuffer(GnmCommandBuffer* commandBuffer)
	{
		m_cb = commandBuffer;
	}

	bool GnmCommandProcessor::processCmdInternal(const void* commandBuffer, uint32_t commandSize)
	{
		bool bRet = false;
		do
		{
			// Note:
			// If something went unusual here, like you found many zero dwords or TYPE0 packets
			// it's likely because there are some GnmDriver functions not implemented,
			// so no proper private packets being inserted into the command buffer.

			const PM4_HEADER* pm4Hdr           = reinterpret_cast<const PM4_HEADER*>(commandBuffer);
			uint32_t          processedCmdSize = 0;

			while (processedCmdSize < commandSize)
			{
				uint32_t pm4Type = pm4Hdr->type;

				switch (pm4Type)
				{
				case PM4_TYPE_0:
					processPM4Type0((PPM4_TYPE_0_HEADER)pm4Hdr, (uint32_t*)(pm4Hdr + 1));
					break;
				case PM4_TYPE_2:
				{
					// opcode should be 0x80000000, this is an 1 dword NOP
					++pm4Hdr;
					processedCmdSize += sizeof(PPM4_HEADER);
					continue;
				}
					break;
				case PM4_TYPE_3:
					processPM4Type3((PPM4_TYPE_3_HEADER)pm4Hdr, (uint32_t*)(pm4Hdr + 1));
					break;
				default:
					LOG_ERR("Invalid pm4 type %d", pm4Type);
					break;
				}

				if (m_flipPacketDone)
				{
					m_flipPacketDone = false;
					break;
				}

				uint32_t processedPm4Count = 1;

				if (m_skipPm4Count != 0)
				{
					processedPm4Count += m_skipPm4Count;
					m_skipPm4Count = 0;
				}

				const PM4_HEADER* nextPm4Hdr      = getNextNPm4(pm4Hdr, processedPm4Count);
				uint32_t          processedLength = reinterpret_cast<uintptr_t>(nextPm4Hdr) - reinterpret_cast<uintptr_t>(pm4Hdr);
				pm4Hdr                            = nextPm4Hdr;
				processedCmdSize += processedLength;
			}

			bRet = true;
		} while (false);

		return bRet;
	}

	void GnmCommandProcessor::processCommandBuffer(const void* commandBuffer, uint32_t commandSize)
	{
		processCmdInternal(commandBuffer, commandSize);
	}

	void GnmCommandProcessor::processPM4Type0(PPM4_TYPE_0_HEADER pm4Hdr, uint32_t* regDataX)
	{
		LOG_FIXME("Type 0 PM4 packet is not supported.");
	}

	void GnmCommandProcessor::processPM4Type3(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		IT_OpCodeType opcode = (IT_OpCodeType)pm4Hdr->opcode;

		// LOG_DEBUG("OpCode Name %s", opcodeName(*(uint32_t*)pm4Hdr));

		switch (opcode)
		{
		case IT_NOP:
			onNop(pm4Hdr, itBody);
			break;
		case IT_SET_BASE:
			onSetBase(pm4Hdr, itBody);
			break;
		case IT_INDEX_BUFFER_SIZE:
			onIndexBufferSize(pm4Hdr, itBody);
			break;
		case IT_SET_PREDICATION:
			onSetPredication(pm4Hdr, itBody);
			break;
		case IT_COND_EXEC:
			onCondExec(pm4Hdr, itBody);
			break;
		case IT_INDEX_BASE:
			onIndexBase(pm4Hdr, itBody);
			break;
		case IT_INDEX_TYPE:
			onIndexType(pm4Hdr, itBody);
			break;
		case IT_NUM_INSTANCES:
			onNumInstances(pm4Hdr, itBody);
			break;
		case IT_STRMOUT_BUFFER_UPDATE:
			onStrmoutBufferUpdate(pm4Hdr, itBody);
			break;
		case IT_WRITE_DATA:
			onWriteData(pm4Hdr, itBody);
			break;
		case IT_MEM_SEMAPHORE:
			onMemSemaphore(pm4Hdr, itBody);
			break;
		case IT_WAIT_REG_MEM:
			onWaitRegMem(pm4Hdr, itBody);
			break;
		case IT_INDIRECT_BUFFER:
			onIndirectBuffer(pm4Hdr, itBody);
			break;
		case IT_PFP_SYNC_ME:
			onPfpSyncMe(pm4Hdr, itBody);
			break;
		case IT_EVENT_WRITE:
			onEventWrite(pm4Hdr, itBody);
			break;
		case IT_EVENT_WRITE_EOP:
			onEventWriteEop(pm4Hdr, itBody);
			break;
		case IT_EVENT_WRITE_EOS:
			onEventWriteEos(pm4Hdr, itBody);
			break;
		case IT_DMA_DATA:
			onDmaData(pm4Hdr, itBody);
			break;
		case IT_ACQUIRE_MEM:
			onAcquireMem(pm4Hdr, itBody);
			break;
		case IT_REWIND:
			onRewind(pm4Hdr, itBody);
			break;
		case IT_SET_CONFIG_REG:
			onSetConfigReg(pm4Hdr, itBody);
			break;
		case IT_SET_CONTEXT_REG:  // 0x69
			onSetContextReg(pm4Hdr, itBody);
			break;
		case IT_SET_SH_REG:
			onSetShReg(pm4Hdr, itBody);
			break;
		case IT_SET_UCONFIG_REG:  // 0x79
			onSetUconfigReg(pm4Hdr, itBody);
			break;
		case IT_INCREMENT_DE_COUNTER:
			onIncrementDeCounter(pm4Hdr, itBody);
			break;
		case IT_WAIT_ON_CE_COUNTER:
			onWaitOnCeCounter(pm4Hdr, itBody);
			break;
		case IT_DISPATCH_DRAW_PREAMBLE__GFX09:
			onDispatchDrawPreambleGfx09(pm4Hdr, itBody);
			break;
		case IT_DISPATCH_DRAW__GFX09:
			onDispatchDrawGfx09(pm4Hdr, itBody);
			break;
		case IT_GET_LOD_STATS__GFX09:
			onGetLodStatsGfx09(pm4Hdr, itBody);
			break;
		case IT_RELEASE_MEM:
			onReleaseMem(pm4Hdr, itBody);
			break;
		// Private handler
		case IT_GNM_PRIVATE:
			onGnmPrivate(pm4Hdr, itBody);
			break;

		// Legacy packets used in old SDKs.
		case IT_DRAW_INDEX_AUTO:
		case IT_DISPATCH_DIRECT:
			onGnmLegacy(pm4Hdr, itBody);
			break;

		// The following opcode types are not used by Gnm

		// TODO:
		// There maybe still some opcodes belongs to Gnm that is not found.
		// We should find all and place them above.
		case IT_CLEAR_STATE:
		case IT_DISPATCH_INDIRECT:
		case IT_INDIRECT_BUFFER_END:
		case IT_INDIRECT_BUFFER_CNST_END:
		case IT_ATOMIC_GDS:
		case IT_ATOMIC_MEM:
		case IT_OCCLUSION_QUERY:
		case IT_REG_RMW:
		case IT_PRED_EXEC:
		case IT_DRAW_INDIRECT:
		case IT_DRAW_INDEX_INDIRECT:
		case IT_DRAW_INDEX_2:
		case IT_CONTEXT_CONTROL:
		case IT_DRAW_INDIRECT_MULTI:
		case IT_DRAW_INDEX_MULTI_AUTO:
		case IT_INDIRECT_BUFFER_PRIV:
		case IT_INDIRECT_BUFFER_CNST:
		case IT_DRAW_INDEX_OFFSET_2:
		case IT_DRAW_PREAMBLE:
		case IT_DRAW_INDEX_INDIRECT_MULTI:
		case IT_DRAW_INDEX_MULTI_INST:
		case IT_COPY_DW:
		case IT_COPY_DATA:
		case IT_CP_DMA:
		case IT_SURFACE_SYNC:
		case IT_ME_INITIALIZE:
		case IT_COND_WRITE:
		case IT_PREAMBLE_CNTL:
		case IT_DRAW_RESERVED0:
		case IT_DRAW_RESERVED1:
		case IT_DRAW_RESERVED2:
		case IT_DRAW_RESERVED3:
		case IT_CONTEXT_REG_RMW:
		case IT_GFX_CNTX_UPDATE:
		case IT_BLK_CNTX_UPDATE:
		case IT_INCR_UPDT_STATE:
		case IT_INTERRUPT:
		case IT_GEN_PDEPTE:
		case IT_INDIRECT_BUFFER_PASID:
		case IT_PRIME_UTCL2:
		case IT_LOAD_UCONFIG_REG:
		case IT_LOAD_SH_REG:
		case IT_LOAD_CONFIG_REG:
		case IT_LOAD_CONTEXT_REG:
		case IT_LOAD_COMPUTE_STATE:
		case IT_LOAD_SH_REG_INDEX:
		case IT_SET_CONTEXT_REG_INDEX:
		case IT_SET_VGPR_REG_DI_MULTI:
		case IT_SET_SH_REG_DI:
		case IT_SET_CONTEXT_REG_INDIRECT:
		case IT_SET_SH_REG_DI_MULTI:
		case IT_GFX_PIPE_LOCK:
		case IT_SET_SH_REG_OFFSET:
		case IT_SET_QUEUE_REG:
		case IT_SET_UCONFIG_REG_INDEX:
		case IT_FORWARD_HEADER:
		case IT_SCRATCH_RAM_WRITE:
		case IT_SCRATCH_RAM_READ:
		case IT_LOAD_CONST_RAM:
		case IT_WRITE_CONST_RAM:
		case IT_DUMP_CONST_RAM:
		case IT_INCREMENT_CE_COUNTER:
		case IT_WAIT_ON_DE_COUNTER_DIFF:
		case IT_SWITCH_BUFFER:
		case IT_FRAME_CONTROL:
		case IT_INDEX_ATTRIBUTES_INDIRECT:
		case IT_WAIT_REG_MEM64:
		case IT_COND_PREEMPT:
		case IT_HDP_FLUSH:
		case IT_INVALIDATE_TLBS:
		case IT_DMA_DATA_FILL_MULTI:
		case IT_SET_SH_REG_INDEX:
		case IT_DRAW_INDIRECT_COUNT_MULTI:
		case IT_DRAW_INDEX_INDIRECT_COUNT_MULTI:
		case IT_DUMP_CONST_RAM_OFFSET:
		case IT_LOAD_CONTEXT_REG_INDEX:
		case IT_SET_RESOURCES:
		case IT_MAP_PROCESS:
		case IT_MAP_QUEUES:
		case IT_UNMAP_QUEUES:
		case IT_QUERY_STATUS:
		case IT_RUN_LIST:
		case IT_MAP_PROCESS_VM:
		case IT_DRAW_MULTI_PREAMBLE__GFX09:
		case IT_AQL_PACKET__GFX09:
			LOG_ERR("Opcode not supported %X", opcode);
			break;
		default:
			LOG_ERR("Invalid opcode %X", opcode);
			break;
		}
	}

	// NOP packet usually used for providing a hint for the following packet,
	// or used for some platform specific operations that do not have a standard
	// opcode, like prepareFlip
	void GnmCommandProcessor::onNop(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		// TODO:
		// Here we should handle allocateFromCommandBuffer and other calls
		// 
		// And a better way to handle nop should check the if upper 16 bits of itBody is equal to 0x6875
		// then dispatch the lower 16 bits
		uint32_t hint = *itBody;
		switch (hint)
		{
		case OP_HINT_SET_VSHARP_IN_USER_DATA:
		case OP_HINT_SET_TSHARP_IN_USER_DATA:
		case OP_HINT_SET_SSHARP_IN_USER_DATA:
		case OP_HINT_SET_USER_DATA_REGION:
			m_lastHint = hint;
			break;
		case OP_HINT_PREPARE_FLIP_VOID:
		case OP_HINT_PREPARE_FLIP_LABEL:
		case OP_HINT_PREPARE_FLIP_WITH_EOP_INTERRUPT_VOID:
		case OP_HINT_PREPARE_FLIP_WITH_EOP_INTERRUPT_LABEL:
			onPrepareFlipOrEopInterrupt(pm4Hdr, itBody);
			break;
		default:
			break;
		}

		if ((hint & 0xFFF) == 0)
		{
			LOG_SCE_GRAPHIC("Gnm: allocateFromCommandBuffer");
			uint32_t align             = (hint - 0x68750000) >> 12;
			uint32_t alignInBytes      = 1 << align;
			uint32_t packetSizeInBytes = PM4_LENGTH_DW(pm4Hdr->u32All) * 4;
			uint32_t size              = (uintptr_t)pm4Hdr + packetSizeInBytes - (((uintptr_t)pm4Hdr + alignInBytes + 7) & ~(alignInBytes - 1));
			m_cb->allocateFromCommandBuffer(size, (EmbeddedDataAlignment)align);
		}
	}

	void GnmCommandProcessor::onSetBase(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onIndexBufferSize(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onSetPredication(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onCondExec(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onIndexBase(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onIndexType(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		uint32_t value = itBody[0];

		IndexSize idxSize = static_cast<IndexSize>(value & 0x3F);

		CachePolicy policy;
		bool        notCachePolicyBypass = bit::extract(value, 10, 10);
		if (notCachePolicyBypass)
		{
			policy = static_cast<CachePolicy>(bit::extract(value, 7, 6));
		}
		else
		{
			policy = kCachePolicyBypass;
		}

		LOG_SCE_GRAPHIC("Gnm: setIndexSize");
		m_cb->setIndexSize(idxSize, policy);
	}

	void GnmCommandProcessor::onNumInstances(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onStrmoutBufferUpdate(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onWriteData(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_WRITE_DATA packet       = (PPM4ME_WRITE_DATA)pm4Hdr;
		void*             dstGpuAddr   = reinterpret_cast<void*>(util::concat<uint64_t>(packet->dstAddrHi, packet->dstAddrLo));
		const void*       data         = &itBody[3];
		uint32_t          sizeInDwords = (packet->header.count + 2) - 4;

		switch (packet->dstSel)
		{
		case dst_sel__me_write_data__memory:
			LOG_SCE_GRAPHIC("Gnm: writeDataInline");
			m_cb->writeDataInline(dstGpuAddr, data, sizeInDwords, (WriteDataConfirmMode)packet->wrConfirm);
			break;
		case dst_sel__me_write_data__tc_l2:
			LOG_SCE_GRAPHIC("Gnm: writeDataInlineThroughL2");
			m_cb->writeDataInlineThroughL2(dstGpuAddr, data, sizeInDwords, (CachePolicy)packet->cachePolicy__CI, (WriteDataConfirmMode)packet->wrConfirm);
			break;
		default:
			LOG_FIXME("Not implemented.");
			break;
		}
	}

	void GnmCommandProcessor::onMemSemaphore(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onWaitRegMem(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_WAIT_REG_MEM packet = (PPM4ME_WAIT_REG_MEM)pm4Hdr;

		void* gpuAddr = reinterpret_cast<void*>(util::concat<uint64_t>(packet->pollAddressHi, packet->pollAddressLo));
		switch (packet->engine)
		{
		case engine_sel__me_wait_reg_mem__me:
			LOG_SCE_GRAPHIC("Gnm: waitOnAddress");
			m_cb->waitOnAddress(gpuAddr, packet->mask, (WaitCompareFunc)packet->function, packet->reference);
			break;
		case engine_sel__me_wait_reg_mem__pfp:
			LOG_SCE_GRAPHIC("Gnm: waitOnAddressAndStallCommandBufferParser");
			m_cb->waitOnAddressAndStallCommandBufferParser(gpuAddr, packet->mask, packet->reference);
			break;
		case engine_sel__me_wait_reg_mem__ce:
			LOG_FIXME("Not implemented.");
			break;
		default:
			break;
		}
	}

	void GnmCommandProcessor::onIndirectBuffer(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4MD_CMD_INDIRECT_BUFFER packet = (PPM4MD_CMD_INDIRECT_BUFFER)pm4Hdr;

		void*    command = reinterpret_cast<void*>(util::concat<uint64_t>(packet->ibBaseHi32, packet->ibBaseLo));
		uint32_t size    = packet->VI.ibSize * sizeof(uint32_t);

		uint32_t oldHint = m_lastHint;
		m_lastHint       = 0;

		LOG_SCE_GRAPHIC("Gnm: callCommandBuffer");
		LOG_SCE_GRAPHIC("Gnm: (((((((((((((((((");
		processCmdInternal(command, size);
		LOG_SCE_GRAPHIC("Gnm: )))))))))))))))))");

		m_lastHint = oldHint;
	}

	void GnmCommandProcessor::onPfpSyncMe(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onEventWrite(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		LOG_SCE_GRAPHIC("Gnm: triggerEvent");
		uint32_t value = itBody[0];
		uint32_t type  = bit::extract(value, 5, 0);
		m_cb->triggerEvent((EventType)type);
	}

	void GnmCommandProcessor::onEventWriteEop(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{

		PPM4_ME_EVENT_WRITE_EOP eopPacket = (PPM4_ME_EVENT_WRITE_EOP)pm4Hdr;

		// From IDA
		eopPacket->ordinal2 -= 0x500;

		// dstSel uses packet's reserved fields
		uint8_t dstSel = ((eopPacket->ordinal4 >> 16) & 1) | ((eopPacket->ordinal2 >> 23) & 0b10);

		// TODO:
		// this is a GPU relative address lacking of the highest byte (masked by 0xFFFFFFFFF8 or 0xFFFFFFFFFC)
		// I'm not sure this relative to what, maybe to the command buffer.
		void* gpuAddr = reinterpret_cast<void*>(util::concat<uint64_t>(eopPacket->addressHi, eopPacket->addressLo));

		uint64_t immValue    = util::concat<uint64_t>(eopPacket->dataHi, eopPacket->dataLo);
		uint8_t  cacheAction = (eopPacket->ordinal2 >> 12) & 0x3F;

		if (eopPacket->intSel)
		{
			LOG_SCE_GRAPHIC("Gnm: writeAtEndOfPipeWithInterrupt");
			m_cb->writeAtEndOfPipeWithInterrupt((EndOfPipeEventType)eopPacket->eventType,
												(EventWriteDest)dstSel, gpuAddr,
												(EventWriteSource)eopPacket->dataSel, immValue,
												(CacheAction)cacheAction, (CachePolicy)eopPacket->cachePolicy__CI);
		}
		else
		{
			LOG_SCE_GRAPHIC("Gnm: writeAtEndOfPipe");
			m_cb->writeAtEndOfPipe((EndOfPipeEventType)eopPacket->eventType,
								   (EventWriteDest)dstSel, gpuAddr,
								   (EventWriteSource)eopPacket->dataSel, immValue,
								   (CacheAction)cacheAction, (CachePolicy)eopPacket->cachePolicy__CI);
		}
	}

	void GnmCommandProcessor::onEventWriteEos(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4_ME_EVENT_WRITE_EOS packet     = (PPM4_ME_EVENT_WRITE_EOS)pm4Hdr;
		uint64_t                dstGpuAddr = util::concat<uint64_t>(packet->addressHi, packet->addressLo);

		LOG_SCE_GRAPHIC("Gnm: writeAtEndOfShader");
		m_cb->writeAtEndOfShader((EndOfShaderEventType)packet->eventType, reinterpret_cast<void*>(dstGpuAddr), packet->data);
	}

	void GnmCommandProcessor::onDmaData(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_DMA_DATA        dmaData      = (PPM4ME_DMA_DATA)pm4Hdr;
		uint32_t               hint         = dmaData->ordinal2;
		switch (hint)
		{
			case OP_HINT_WRITE_GPU_PREFETCH_INTO_L2:
			{
				if (dmaData->dst_addr_lo != OP_HINT_WRITE_GPU_PREFETCH_INTO_L2_2)
				{
					break;
				}
				LOG_SCE_GRAPHIC("Gnm: prefetchIntoL2");
				uint64_t addr = util::concat<uint64_t>(dmaData->src_addr_hi, dmaData->src_addr_lo_or_data);
				m_cb->prefetchIntoL2((void*)addr, dmaData->bitfields7.byte_count);
			}
				break;
		}
	}

	void GnmCommandProcessor::onAcquireMem(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_ACQUIRE_MEM__GFX09 packet = (PPM4ME_ACQUIRE_MEM__GFX09)pm4Hdr;

		uint32_t                     value             = packet->ordinal2;
		StallCommandBufferParserMode stallMode         = (StallCommandBufferParserMode)bit::extract(value, 31, 31);
		uint32_t                     targetMask        = bit::extract(value, 14, 0);
		uint32_t                     extendedCacheMask = bit::extract(value, 29, 24) << 24;
		uint32_t                     cacheAction       = (bit::extract(value, 23, 22) << 4) | bit::extract(value, 18, 15);
		uint32_t                     baseAddr256       = packet->coher_base_lo;
		if (baseAddr256)
		{
			LOG_SCE_GRAPHIC("Gnm: waitForGraphicsWrites");
			m_cb->waitForGraphicsWrites(baseAddr256, packet->coher_size, targetMask, (CacheAction)cacheAction, extendedCacheMask, stallMode);
		}
		else
		{
			LOG_SCE_GRAPHIC("Gnm: flushShaderCachesAndWait");
			m_cb->flushShaderCachesAndWait((CacheAction)cacheAction, extendedCacheMask, stallMode);
		}
	}

	void GnmCommandProcessor::onRewind(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onSetConfigReg(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onSetContextReg(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_SET_CONTEXT_REG setCtxPacket = (PPM4ME_SET_CONTEXT_REG)pm4Hdr;
		uint32_t               regOffset    = setCtxPacket->bitfields2.reg_offset;
		uint32_t               hint         = regOffset;

		switch (hint)
		{
		case OP_HINT_SET_DB_RENDER_CONTROL:
		{
			DbRenderControl drc = {};
			drc.m_reg           = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setDbRenderControl");
			m_cb->setDbRenderControl(drc);
		}
		break;
		case OP_HINT_SET_PS_SHADER_USAGE:
		{
			const uint32_t* inputTable = &itBody[1];
			const uint32_t  numItems   = pm4Hdr->count;
			LOG_SCE_GRAPHIC("Gnm: setPsShaderUsage");
			m_cb->setPsShaderUsage(inputTable, numItems);
		}
		break;
		case OP_HINT_SET_VIEWPORT_TRANSFORM_CONTROL:
		{
			ViewportTransformControl vpc = {};
			vpc.m_reg                    = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setViewportTransformControl");
			m_cb->setViewportTransformControl(vpc);
		}
		break;
		case OP_HINT_SET_SCREEN_SCISSOR:
		{
			int32_t left   = bit::extract(itBody[1], 15, 0);
			int32_t top    = bit::extract(itBody[1], 31, 16);
			int32_t right  = bit::extract(itBody[2], 15, 0);
			int32_t bottom = bit::extract(itBody[2], 31, 16);
			LOG_SCE_GRAPHIC("Gnm: setScreenScissor");
			m_cb->setScreenScissor(left, top, right, bottom);
		}
		break;
		case OP_HINT_SET_HARDWARE_SCREEN_OFFSET:
		{
			uint32_t offsetX = bit::extract(itBody[1], 15, 0);
			uint32_t offsetY = bit::extract(itBody[1], 31, 16);
			LOG_SCE_GRAPHIC("Gnm: setHardwareScreenOffset");
			m_cb->setHardwareScreenOffset(offsetX, offsetY);
		}
		break;
		case OP_HINT_SET_GUARD_BANDS:
		{
			float vertClip    = *reinterpret_cast<float*>(&itBody[1]);
			float vertDiscard = *reinterpret_cast<float*>(&itBody[2]);
			float horzClip    = *reinterpret_cast<float*>(&itBody[3]);
			float horzDiscard = *reinterpret_cast<float*>(&itBody[4]);
			LOG_SCE_GRAPHIC("Gnm: setGuardBands");
			m_cb->setGuardBands(horzClip, vertClip, horzDiscard, vertDiscard);
		}
		break;
		case OP_HINT_SET_DEPTH_RENDER_TARGET:
		{
			auto nextPm4 = getNextPm4(pm4Hdr);
			if (nextPm4->opcode == IT_SET_CONTEXT_REG &&
				(((uint32_t*)nextPm4)[1] == 15 ||
				 ((uint32_t*)nextPm4)[1] == 17))
			{
				onSetDepthRenderTarget(pm4Hdr, itBody);
			}
		}
		break;
		case OP_HINT_SET_RENDER_TARGET_MASK:
		{
			uint32_t mask = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setRenderTargetMask");
			m_cb->setRenderTargetMask(mask);
		}
		break;
		case OP_HINT_SET_DEPTH_STENCIL_CONTROL:
		{
			uint32_t reg = itBody[1];
			if (reg)
			{
				DepthStencilControl dsc;
				dsc.m_reg = reg;
				LOG_SCE_GRAPHIC("Gnm: setDepthStencilControl");
				m_cb->setDepthStencilControl(dsc);
			}
			else
			{
				LOG_SCE_GRAPHIC("Gnm: setDepthStencilDisable");
				m_cb->setDepthStencilDisable();
			}
		}
		break;
		case OP_HINT_SET_PRIMITIVE_SETUP:
		{
			PrimitiveSetup primSetupReg;
			primSetupReg.m_reg = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setPrimitiveSetup");
			m_cb->setPrimitiveSetup(primSetupReg);
		}
		break;
		case OP_HINT_SET_ACTIVE_SHADER_STAGES:
		{
			ActiveShaderStages activeStages = static_cast<ActiveShaderStages>(itBody[1]);
			LOG_SCE_GRAPHIC("Gnm: setActiveShaderStages");
			m_cb->setActiveShaderStages(activeStages);
		}
		break;
		case OP_HINT_SET_DEPTH_CLEAR_VALUE:
		{
			float clearValue = *reinterpret_cast<float*>(&itBody[1]);
			LOG_SCE_GRAPHIC("Gnm: setDepthClearValue");
			m_cb->setDepthClearValue(clearValue);
		}
		break;
		case OP_HINT_SET_STENCIL_CLEAR_VALUE:
		{
			uint8_t clearValue = static_cast<uint8_t>(itBody[1]);
			LOG_SCE_GRAPHIC("Gnm: setStencilClearValue");
			m_cb->setStencilClearValue(clearValue);
		}
		break;
		case OP_HINT_SET_CLIP_CONTROL:
		{
			ClipControl clipControl;
			clipControl.m_reg = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setClipControl");
			m_cb->setClipControl(clipControl);
		}
		break;
		case OP_HINT_SET_DB_COUNT_CONTROL:
		{
			uint32_t value = itBody[1];
			value          = value - 0x11000100;
			uint32_t perfectZPassCounts = bit::extract(value, 1, 1);
			uint32_t log2SampleRate     = bit::extract(value, 6, 4);
			LOG_SCE_GRAPHIC("Gnm: setDbCountControl");
			m_cb->setDbCountControl((DbCountControlPerfectZPassCounts)perfectZPassCounts, log2SampleRate);
		}
		break;
		case OP_HINT_SET_BORDER_COLOR_TABLE_ADDR:
		{
			uint32_t value = itBody[1];
			void*    tableAddr = reinterpret_cast<void*>(value << 8);
			LOG_SCE_GRAPHIC("Gnm: setBorderColorTableAddr");
			m_cb->setBorderColorTableAddr(tableAddr);
		}
		break;
		case OP_HINT_SET_STENCIL_OR_SEPARATE:
		{
			StencilControl front, back;
			front.m_reg = itBody[1];
			back.m_reg  = itBody[2];
			if (front.m_reg == back.m_reg)
			{
				LOG_SCE_GRAPHIC("Gnm: setStencil");
				m_cb->setStencil(front);
			}
			else
			{
				LOG_SCE_GRAPHIC("Gnm: setStencilSeparate");
				m_cb->setStencilSeparate(front, back);
			}
		}
		break;
		case OP_HINT_SET_STENCIL_OP_CONTROL:
		{
			StencilOpControl opCtrl;
			opCtrl.m_reg = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setStencilOpControl");
			m_cb->setStencilOpControl(opCtrl);
		}
		break;
		case OP_HINT_SET_CB_CONTROL:
		{
			uint32_t value = itBody[1];
			uint32_t mode  = bit::extract(value, 6, 4);
			uint32_t op    = bit::extract(value, 23, 16);
			LOG_SCE_GRAPHIC("Gnm: setCbControl");
			m_cb->setCbControl((CbMode)mode, (RasterOp)op);
		}
		break;
		}

		if (regOffset >= 0xB4 && regOffset <= 0xD2)
		{
			onSetViewport(pm4Hdr, itBody);
		}
		else if (regOffset >= 0x318 && regOffset <= (0x31C + 15 * 7))
		{
			onSetRenderTarget(pm4Hdr, itBody);
		}
		else if (regOffset >= 0x1E0 && regOffset <= (0x1E0 + 7))
		{
			uint32_t     rtSlot = regOffset - 0x1E0;
			BlendControl bc;
			bc.m_reg = itBody[1];
			LOG_SCE_GRAPHIC("Gnm: setBlendControl");
			m_cb->setBlendControl(rtSlot, bc);
		}
	}

	void GnmCommandProcessor::onSetShReg(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_SET_SH_REG shPacket = (PPM4ME_SET_SH_REG)pm4Hdr;

		if (pm4Hdr->count != 1)
		{
			ShaderStage stage;
			if (pm4Hdr->shaderType)
			{
				stage = kShaderStageCs;
			}
			else
			{
				// This is a trick.
				//0x2C0C - 0x2C00 = 0x00C
				//0x2C4C - 0x2C00 = 0x04C
				//0x2C8C - 0x2C00 = 0x08C
				//0x2CCC - 0x2C00 = 0x0CC
				//0x2D0C - 0x2C00 = 0x10C
				//0x2D4C - 0x2C00 = 0x14C
				// The max value for startSlot is 15 = 0xF
				// And the sub result plus 0xF won't exceed 5 bits
				//(0x00C + 0xF) >> 5 = 0 = 2 * 0
				//(0x04C + 0xF) >> 5 = 2 = 2 * 1
				//(0x08C + 0xF) >> 5 = 4 = 2 * 2
				//(0x0CC + 0xF) >> 5 = 6 = 2 * 3
				//(0x10C + 0xF) >> 5 = 8 = 2 * 4
				//(0x14C + 0xF) >> 5 = 10 = 2 * 5
				stage = (ShaderStage)(((shPacket->bitfields2.reg_offset >> 5) / 2) + 1);
			}

			uint32_t stageBase = c_stageBases[stage];
			uint32_t startSlot = shPacket->bitfields2.reg_offset + 0x2C00 - stageBase;
			void*    gpuAddr   = reinterpret_cast<void*>(itBody + 1);

			bool isPointer = false;
			switch (m_lastHint)
			{
			case OP_HINT_SET_VSHARP_IN_USER_DATA:
				LOG_SCE_GRAPHIC("Gnm: setVsharpInUserData");
				m_cb->setVsharpInUserData(stage, startSlot, (const Buffer*)gpuAddr);
				break;
			case OP_HINT_SET_TSHARP_IN_USER_DATA:
				LOG_SCE_GRAPHIC("Gnm: setTsharpInUserData");
				m_cb->setTsharpInUserData(stage, startSlot, (const Texture*)gpuAddr);
				break;
			case OP_HINT_SET_SSHARP_IN_USER_DATA:
				LOG_SCE_GRAPHIC("Gnm: setSsharpInUserData");
				m_cb->setSsharpInUserData(stage, startSlot, (const Sampler*)gpuAddr);
				break;
			case OP_HINT_SET_USER_DATA_REGION:
				LOG_SCE_GRAPHIC("Gnm: setUserDataRegion");
				m_cb->setUserDataRegion(stage, startSlot, &itBody[1], pm4Hdr->count);
				break;
			default:
				isPointer = true;
				break;
			}

			if (isPointer && pm4Hdr->count == 2)  // 2 for a pointer type size
			{
				LOG_SCE_GRAPHIC("Gnm: setPointerInUserData");
				m_cb->setPointerInUserData(stage, startSlot, gpuAddr);
			}
		}
		else
		{
			LOG_FIXME("Not implemented.");
			uint32_t hint = shPacket->bitfields2.reg_offset;
			if (hint == OP_HINT_SET_COMPUTE_SHADER_CONTROL)
			{
				//m_dcb.setComputeShaderControl();
			}
			else if (hint == OP_HINT_SET_COMPUTE_SCRATCH_SIZE)
			{
				//m_dcb.setComputeScratchSize();
			}
			else if (!pm4Hdr->shaderType && hint >= 0x0C && hint <= 0x14C)
			{
				// non cs
				//m_dcb.setUserData()
			}
			else if (pm4Hdr->shaderType && hint == 0x240)
			{
				//cs
				//m_dcb.setUserData();
			}
			else if (hint >= 0x216 && hint <= 0x21A)
			{
				//m_dcb.setComputeResourceManagement()
			}
		}

		m_lastHint = 0;
	}

	void GnmCommandProcessor::onSetUconfigReg(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_SET_UCONFIG_REG setUcfgPacket = (PPM4ME_SET_UCONFIG_REG)pm4Hdr;

		switch (setUcfgPacket->bitfields2.reg_offset)
		{
		case OP_HINT_SET_PRIMITIVE_TYPE_BASE:
			LOG_SCE_GRAPHIC("Gnm: setPrimitiveType");
			m_cb->setPrimitiveType((PrimitiveType)itBody[1]);
			break;
		default:
			break;
		}
	}

	void GnmCommandProcessor::onIncrementDeCounter(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onWaitOnCeCounter(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onDispatchDrawPreambleGfx09(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onDispatchDrawGfx09(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onGetLodStatsGfx09(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
	}

	void GnmCommandProcessor::onReleaseMem(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_RELEASE_MEM__GFX09 packet = (PPM4ME_RELEASE_MEM__GFX09)pm4Hdr;

		ReleaseMemEventType eventType   = (ReleaseMemEventType)packet->event_type;
		EventWriteDest      dstSelector = (EventWriteDest)packet->dstSel;
		void*               dstGpuAddr  = reinterpret_cast<void*>(util::concat<uint64_t>(packet->addressHi, packet->addressLo));
		EventWriteSource    srcSelector = (EventWriteSource)packet->dataSel;
		uint64_t            immValue    = util::concat<uint64_t>(packet->dataHi, packet->dataLo);
		CacheAction         cacheAction = (CacheAction)packet->cache_action;
		CachePolicy         writePolicy = (CachePolicy)packet->cache_policy;
		if (packet->intSel == int_sel__me_release_mem__none ||
			packet->intSel == int_sel__me_release_mem__send_data_and_write_confirm)
		{
			LOG_SCE_GRAPHIC("Gnm: writeReleaseMemEvent");
			m_cb->writeReleaseMemEvent(
				eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
		}
		else
		{
			LOG_SCE_GRAPHIC("Gnm: writeReleaseMemEventWithInterrupt");
			m_cb->writeReleaseMemEventWithInterrupt(
				eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
		}
	}

	void GnmCommandProcessor::onGnmPrivate(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		// Note:
		// Most private opcode handle cases are not much complicated,
		// just cast pm4Hdr to proper GnmCmdxxx and call the graphic function.
		// But if it's going to be complicated (eg. contains 'if' statement),
		// please write a new function to handle it,
		// like onDrawIndex

		IT_OpCodePriv priv = PM4_PRIV(*(uint32_t*)pm4Hdr);
		switch (priv)
		{
			case OP_PRIV_INITIALIZE_DEFAULT_HARDWARE_STATE:
				LOG_SCE_GRAPHIC("Gnm: initializeDefaultHardwareState");
				m_cb->initializeDefaultHardwareState();
				break;
			case OP_PRIV_INITIALIZE_TO_DEFAULT_CONTEXT_STATE:
				break;
			case OP_PRIV_SET_EMBEDDED_VS_SHADER:
			{
				GnmCmdVSShader* param = (GnmCmdVSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: setEmbeddedVsShader");
				m_cb->setEmbeddedVsShader(param->shaderId, param->modifier);
			}
			break;
			case OP_PRIV_SET_EMBEDDED_PS_SHADER:
				break;
			case OP_PRIV_SET_VS_SHADER:
			{
				GnmCmdVSShader* param = (GnmCmdVSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: setVsShader");
				m_cb->setVsShader(&param->vsRegs, param->modifier);
			}
			break;
			case OP_PRIV_SET_PS_SHADER:
			{
				GnmCmdPSShader* param = (GnmCmdPSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: setPsShader");
				m_cb->setPsShader(&param->psRegs);
			}
			break;
			case OP_PRIV_SET_CS_SHADER:
			{
				GnmCmdCSShader* param = (GnmCmdCSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: setCsShader");
				m_cb->setCsShader(&param->csRegs, param->modifier);
			}
			break;
			case OP_PRIV_SET_ES_SHADER:
				break;
			case OP_PRIV_SET_GS_SHADER:
				break;
			case OP_PRIV_SET_HS_SHADER:
				break;
			case OP_PRIV_SET_LS_SHADER:
				break;
			case OP_PRIV_UPDATE_GS_SHADER:
				break;
			case OP_PRIV_UPDATE_HS_SHADER:
				break;
			case OP_PRIV_UPDATE_PS_SHADER:
			{
				GnmCmdPSShader* param = (GnmCmdPSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: updatePsShader");
				m_cb->updatePsShader(&param->psRegs);
			}
			break;
			case OP_PRIV_UPDATE_VS_SHADER:
			{
				GnmCmdVSShader* param = (GnmCmdVSShader*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: updateVsShader");
				m_cb->updateVsShader(&param->vsRegs, param->modifier);
			}
			break;
			case OP_PRIV_SET_VGT_CONTROL:
			{
				GnmCmdVgtControl* param = (GnmCmdVgtControl*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: setVgtControlForNeo");
				m_cb->setVgtControlForNeo(param->primGroupSizeMinusOne,
										  (WdSwitchOnlyOnEopMode)param->wdSwitchOnlyOnEopMode,
										  (VgtPartialVsWaveMode)param->partialVsWaveMode);
			}
			break;
			case OP_PRIV_RESET_VGT_CONTROL:
				break;
			case OP_PRIV_DRAW_INDEX:
				onDrawIndex(pm4Hdr, itBody);
				break;
			case OP_PRIV_DRAW_INDEX_AUTO:
				onDrawIndexAuto(pm4Hdr, itBody);
				break;
			case OP_PRIV_DRAW_INDEX_INDIRECT:
				break;
			case OP_PRIV_DRAW_INDEX_INDIRECT_COUNT_MULTI:
				break;
			case OP_PRIV_DRAW_INDEX_MULTI_INSTANCED:
				break;
			case OP_PRIV_DRAW_INDEX_OFFSET:
				break;
			case OP_PRIV_DRAW_INDIRECT:
				break;
			case OP_PRIV_DRAW_INDIRECT_COUNT_MULTI:
				break;
			case OP_PRIV_DRAW_OPAQUE_AUTO:
				break;
			case OP_PRIV_WAIT_UNTIL_SAFE_FOR_RENDERING:
			{
				GnmCmdWaitFlipDone* param = (GnmCmdWaitFlipDone*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: waitUntilSafeForRendering");
				m_cb->waitUntilSafeForRendering(param->videoOutHandle, param->displayBufferIndex);
			}
			break;
			case OP_PRIV_PUSH_MARKER:
			{
				GnmCmdPushMarker* param = (GnmCmdPushMarker*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: pushMarker");
				m_cb->pushMarker(param->debugString);
			}
				break;
			case OP_PRIV_PUSH_COLOR_MARKER:
			{
				GnmCmdPushColorMarker* param = (GnmCmdPushColorMarker*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: pushColorMarker");
				m_cb->pushMarker(param->debugString, param->argbColor);
			}
				break;
			case OP_PRIV_POP_MARKER:
			{
				LOG_SCE_GRAPHIC("Gnm: popMarker");
				m_cb->popMarker();
			}
				break;
			case OP_PRIV_SET_MARKER:
				break;
			case OP_PRIV_DISPATCH_DIRECT:
			{
				GnmCmdDispatchDirect*     param = (GnmCmdDispatchDirect*)pm4Hdr;
				DispatchOrderedAppendMode mode  = (DispatchOrderedAppendMode)bit::extract(param->pred, 4, 3);
				if (mode == kDispatchOrderedAppendModeDisabled)
				{
					LOG_SCE_GRAPHIC("Gnm: dispatch");
					m_cb->dispatch(param->threadGroupX, param->threadGroupY, param->threadGroupZ);
				}
				else
				{
					LOG_SCE_GRAPHIC("Gnm: dispatchWithOrderedAppend");
					m_cb->dispatchWithOrderedAppend(param->threadGroupX, param->threadGroupY, param->threadGroupZ, mode);
				}
				LOG_SCE_GRAPHIC("Gnm: ---------------------------------------");
			}
				break;
			case OP_PRIV_DISPATCH_INDIRECT:
				break;
			case OP_PRIV_COMPUTE_WAIT_ON_ADDRESS:
			{
				GnmCmdComputeWaitOnAddress* param = (GnmCmdComputeWaitOnAddress*)pm4Hdr;
				LOG_SCE_GRAPHIC("Gnm: waitOnAddress");
				m_cb->waitOnAddress((void*)param->gpuAddr, param->mask, (WaitCompareFunc)param->compareFunc, param->refValue);
			}
				break;
			default:
				break;
		}
	}

	void GnmCommandProcessor::onGnmLegacy(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		// Some gnm call implementations are different in old SDK libs.
		// They will fill pm4 packets themselves directly,
		// instead of calling gnm driver functions, like newer SDKs do.
		// Thus we miss the chance to fill private packets in command buffer.
		// So we have to handle these kinds of packets here.

		IT_OpCodeType opcode = (IT_OpCodeType)pm4Hdr->opcode;
		switch (opcode)
		{
		case IT_DISPATCH_DIRECT:
		{
			uint32_t threadGroupX = itBody[0];
			uint32_t threadGroupY = itBody[1];
			uint32_t threadGroupZ = itBody[2];
			LOG_SCE_GRAPHIC("Gnm: dispatch");
			m_cb->dispatch(threadGroupX, threadGroupY, threadGroupZ);
			LOG_SCE_GRAPHIC("Gnm: ---------------------------------------");
		}
			break;
		case IT_DRAW_INDEX_AUTO:
		{
			uint32_t indexCount = itBody[0];
			LOG_SCE_GRAPHIC("Gnm: drawIndexAuto");
			m_cb->drawIndexAuto(indexCount);
			LOG_SCE_GRAPHIC("Gnm: ---------------------------------------");
		}
			break;
		default:
			LOG_FIXME("legacy opcode not handled %X", opcode);
			break;
		}
	}

	void GnmCommandProcessor::onPrepareFlipOrEopInterrupt(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		uint32_t hint = itBody[0];

		//void* labelAddr = (void*)((uint64_t(itBody[2]) << 32) | itBody[1]);
		void*    labelAddr = (void*)*(uint64_t*)(itBody + 1);
		uint32_t value     = itBody[3];

		switch (hint)
		{
		case OP_HINT_PREPARE_FLIP_VOID:
			LOG_SCE_GRAPHIC("Gnm: prepareFlip");
			m_cb->prepareFlip();
			break;
		case OP_HINT_PREPARE_FLIP_LABEL:
			LOG_SCE_GRAPHIC("Gnm: prepareFlip");
			m_cb->prepareFlip(labelAddr, value);
			break;
		case OP_HINT_PREPARE_FLIP_WITH_EOP_INTERRUPT_VOID:
			LOG_FIXME("Not implemented.");
			break;
		case OP_HINT_PREPARE_FLIP_WITH_EOP_INTERRUPT_LABEL:
		{
			EndOfPipeEventType eventType   = (EndOfPipeEventType)itBody[4];
			CacheAction        cacheAction = (CacheAction)itBody[5];
			LOG_SCE_GRAPHIC("Gnm: prepareFlipWithEopInterrupt");
			m_cb->prepareFlipWithEopInterrupt(eventType, labelAddr, value, cacheAction);
		}
			break;
		default:
			break;
		}

		LOG_SCE_GRAPHIC("Gnm: =======================================");

		// mark this is the last packet in command buffer.
		m_flipPacketDone = true;
	}

	void GnmCommandProcessor::onDrawIndex(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		LOG_SCE_GRAPHIC("Gnm: drawIndex");
		GnmCmdDrawIndex* param           = (GnmCmdDrawIndex*)pm4Hdr;
		DrawModifier     modifier        = { 0 };
		modifier.renderTargetSliceOffset = (param->predAndMod >> 29) & 0b111;
		if (!modifier.renderTargetSliceOffset)
		{
			m_cb->drawIndex(param->indexCount, (const void*)param->indexAddr);
		}
		else
		{
			m_cb->drawIndex(param->indexCount, (const void*)param->indexAddr, modifier);
		}
		LOG_SCE_GRAPHIC("Gnm: ---------------------------------------");
	}

	void GnmCommandProcessor::onDrawIndexAuto(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		LOG_SCE_GRAPHIC("Gnm: drawIndexAuto");
		GnmCmdDrawIndexAuto* param       = (GnmCmdDrawIndexAuto*)pm4Hdr;
		DrawModifier         modifier    = { 0 };
		modifier.renderTargetSliceOffset = (param->predAndMod >> 29) & 0b111;
		if (!modifier.renderTargetSliceOffset)
		{
			m_cb->drawIndexAuto(param->indexCount);
		}
		else
		{
			m_cb->drawIndexAuto(param->indexCount, modifier);
		}
		LOG_SCE_GRAPHIC("Gnm: ---------------------------------------");
	}

	void GnmCommandProcessor::onSetViewport(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		LOG_SCE_GRAPHIC("Gnm: setViewport");

		float dmin = *reinterpret_cast<float*>(&itBody[1]);
		float dmax = *reinterpret_cast<float*>(&itBody[2]);

		PPM4_TYPE_3_HEADER nextPacket = getNextPm4(pm4Hdr);
		uint32_t*          nextItBody = reinterpret_cast<uint32_t*>(nextPacket + 1);

		float scale[3]  = { 0.0 };
		float offset[3] = { 0.0 };

		scale[0]  = *reinterpret_cast<float*>(&nextItBody[1]);
		scale[1]  = *reinterpret_cast<float*>(&nextItBody[3]);
		scale[2]  = *reinterpret_cast<float*>(&nextItBody[5]);
		offset[0] = *reinterpret_cast<float*>(&nextItBody[2]);
		offset[1] = *reinterpret_cast<float*>(&nextItBody[4]);
		offset[2] = *reinterpret_cast<float*>(&nextItBody[6]);

		uint32_t viewportId = (nextItBody[0] - 0x10F) / 6;
		m_cb->setViewport(viewportId, dmin, dmax, scale, offset);

		// Skip the second packet.
		m_skipPm4Count = 1;
	}

	void GnmCommandProcessor::onSetRenderTarget(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		PPM4ME_SET_CONTEXT_REG setCtxPacket = (PPM4ME_SET_CONTEXT_REG)pm4Hdr;

		uint32_t offset    = setCtxPacket->bitfields2.reg_offset - 0x318;
		bool     isValidRt = (offset % 15 == 0);

		LOG_SCE_GRAPHIC("Gnm: setRenderTarget");

		if (isValidRt)
		{
			uint32_t rtSlot = (setCtxPacket->bitfields2.reg_offset - 0x318) / 15;

			RenderTarget target;
			std::memcpy(&target.m_regs[0], &itBody[1], sizeof(uint32_t) * setCtxPacket->header.count);

			// Get the next nop packet, which is used to hold width and height information.
			auto      nopPacket                         = getNextPm4(pm4Hdr);
			uint32_t* nopBody                           = reinterpret_cast<uint32_t*>(nopPacket) + 1;
			uint32_t  packWidthHeight                   = nopBody[0];
			target.m_regs[RenderTarget::kCbWidthHeight] = packWidthHeight;

			m_cb->setRenderTarget(rtSlot, &target);

			// Skip the nop packet
			m_skipPm4Count = 1;
		}
		else
		{
			// The game may set an empty render target.
			uint32_t rtSlot = (setCtxPacket->bitfields2.reg_offset - 0x31C) / 15;
			m_cb->setRenderTarget(rtSlot, nullptr);
		}
	}

	void GnmCommandProcessor::onSetDepthRenderTarget(PPM4_TYPE_3_HEADER pm4Hdr, uint32_t* itBody)
	{
		LOG_SCE_GRAPHIC("Gnm: setDepthRenderTarget");
		PPM4ME_SET_CONTEXT_REG nextPacket = (PPM4ME_SET_CONTEXT_REG)getNextPm4(pm4Hdr);
		if (nextPacket->bitfields2.reg_offset == 15)
		{
			DepthRenderTarget target;

			std::memcpy(&target.m_regs[0], &itBody[1], 0x20);
			
			uint32_t* cmdptr  = (uint32_t*)pm4Hdr;
			target.m_regs[11] = cmdptr[12];
			target.m_regs[8]  = cmdptr[15];
			target.m_regs[9]  = cmdptr[18];
			target.m_regs[10] = cmdptr[21];
			target.m_regs[12] = cmdptr[23];

			m_cb->setDepthRenderTarget(&target);
			m_skipPm4Count = 5;
		}
		else if (nextPacket->bitfields2.reg_offset == 17)
		{
			m_cb->setDepthRenderTarget(nullptr);
			m_skipPm4Count = 1;
		}
		else
		{
			LOG_ERR("error set depth render target packet, next reg off %d", nextPacket->bitfields2.reg_offset);
		}
	}

}  // namespace sce::Gnm
