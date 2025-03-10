#pragma once

#include "GcnCommon.h"
#include "GcnModInfo.h"
#include "GcnProgramInfo.h"
#include "GcnInstructionIterator.h"
#include "GcnShaderMeta.h"
#include "GcnDecoder.h"
#include "GcnStateRegister.h"
#include "GcnCompilerDefs.h"

#include "SpirV/SpirvModule.h"
#include "Violet/VltRc.h"
#include "Violet/VltPipeLayout.h"
#include "Violet/VltShader.h"

#include <vector>
#include <unordered_map>

namespace sce::Gnm
{
	enum TextureType;
}  // namespace sce::Gnm

namespace sce::gcn
{
	class GcnToken;
	class GcnTokenList;
	class GcnHeader;
	struct GcnTokenCondition;
	struct GcnShaderInstruction;
	struct GcnAnalysisInfo;
	struct GcnShaderResource;


	/**
	 * \brief Shader recompiler
	 * 
	 * Recompile GCN instructions into Spir-V byte code.
	 * Produce VltShader for Violet.
	 */
	class GcnCompiler : public GcnInstructionIterator
	{
		friend class GcnStateRegister;

	public:
		GcnCompiler(
			const std::string&     fileName,
			const GcnModuleInfo&   moduleInfo,
			const GcnProgramInfo&  programInfo,
			const GcnHeader&       header,
			const GcnShaderMeta&   meta,
			const GcnAnalysisInfo& analysis);
		virtual ~GcnCompiler();

		/**
		 * \brief Compile tokens into spirv
		 * \param [in] tokens The token list
		 */
		void compile(const GcnTokenList& tokens);

		/**
         * \brief Finalizes the shader
         * \returns The final shader object
         */
		vlt::Rc<vlt::VltShader> finalize();

	private:
		void compileToken(const GcnToken& token);
		void compileTokenCode(const GcnToken& token);
		void compileTokenLoop(const GcnToken& token);
		void compileTokenBlock(const GcnToken& token);
		void compileTokenIf(const GcnToken& token);
		void compileTokenIfNot(const GcnToken& token);
		void compileTokenElse(const GcnToken& token);
		void compileTokenBranch(const GcnToken& token);
		void compileTokenEnd(const GcnToken& token);
		void compileTokenVariable(const GcnToken& token);
		void compileTokenSetValue(const GcnToken& token);
		void compileGlobalVariable(const GcnTokenList& tokens);

		void compileInstruction(
			const GcnShaderInstruction& ins);
		/////////////////////////////////////
		// Control flow instruction handlers
		void emitControlFlowIf(const GcnToken& token);
		void emitControlFlowElse(const GcnToken& token);
		void emitControlFlowEndIf(const GcnToken& token);
		void emitControlFlowLoop(const GcnToken& token);
		void emitControlFlowEndLoop(const GcnToken& token);
		void emitControlFlowBlock(const GcnToken& token);
		void emitControlFlowEndBlock(const GcnToken& token);
		void emitControlFlowBreak(const GcnToken& token);
		void emitControlFlowReturn();
		void emitControlFlowDiscard();

		uint32_t emitControlFlowCondition(const GcnTokenCondition& condition);
		uint32_t emitComputeDivergence();
		uint32_t emitControlFlowDivergence();
		/////////////////////////////////////////////////////////
		// Category handlers
		void emitScalarALU(const GcnShaderInstruction& ins);
		void emitScalarMemory(const GcnShaderInstruction& ins);
		void emitVectorALU(const GcnShaderInstruction& ins);
		void emitVectorMemory(const GcnShaderInstruction& ins);
		void emitFlowControl(const GcnShaderInstruction& ins);
		void emitDataShare(const GcnShaderInstruction& ins);
		void emitVectorInterpolation(const GcnShaderInstruction& ins);
		void emitExport(const GcnShaderInstruction& ins);
		void emitDebugProfile(const GcnShaderInstruction& ins);

		/////////////////////////////////////////////////////////
		// Class handlers
		void emitScalarAluCommon(const GcnShaderInstruction& ins);
		void emitScalarArith(const GcnShaderInstruction& ins);
		void emitScalarAbs(const GcnShaderInstruction& ins);
		void emitScalarMov(const GcnShaderInstruction& ins);
		void emitScalarMovRel(const GcnShaderInstruction& ins);
		void emitScalarCmp(const GcnShaderInstruction& ins);
		void emitScalarSelect(const GcnShaderInstruction& ins);
		void emitScalarBitLogic(const GcnShaderInstruction& ins);
		void emitScalarBitManip(const GcnShaderInstruction& ins);
		void emitScalarBitField(const GcnShaderInstruction& ins);
		void emitScalarConv(const GcnShaderInstruction& ins);
		void emitScalarExecMask(const GcnShaderInstruction& ins);
		void emitScalarQuadMask(const GcnShaderInstruction& ins);
		void emitVectorAluCommon(const GcnShaderInstruction& ins);
		void emitVectorCmp(const GcnShaderInstruction& ins);
		void emitVectorRegMov(const GcnShaderInstruction& ins);
		void emitVectorMovRel(const GcnShaderInstruction& ins);
		void emitVectorLane(const GcnShaderInstruction& ins);
		void emitVectorBitLogic(const GcnShaderInstruction& ins);
		void emitVectorBitField32(const GcnShaderInstruction& ins);
		void emitVectorThreadMask(const GcnShaderInstruction& ins);
		void emitVectorBitField64(const GcnShaderInstruction& ins);
		void emitVectorFpArith32(const GcnShaderInstruction& ins);
		void emitVectorFpRound32(const GcnShaderInstruction& ins);
		void emitVectorFpField32(const GcnShaderInstruction& ins);
		void emitVectorFpTran32(const GcnShaderInstruction& ins);
		void emitVectorFpCmp32(const GcnShaderInstruction& ins);
		void emitVectorFpArith64(const GcnShaderInstruction& ins);
		void emitVectorFpRound64(const GcnShaderInstruction& ins);
		void emitVectorFpField64(const GcnShaderInstruction& ins);
		void emitVectorFpTran64(const GcnShaderInstruction& ins);
		void emitVectorFpCmp64(const GcnShaderInstruction& ins);
		void emitVectorIntArith32(const GcnShaderInstruction& ins);
		void emitVectorIntArith64(const GcnShaderInstruction& ins);
		void emitVectorIntCmp32(const GcnShaderInstruction& ins);
		void emitVectorIntCmp64(const GcnShaderInstruction& ins);
		void emitVectorConv(const GcnShaderInstruction& ins);
		void emitVectorFpGraph32(const GcnShaderInstruction& ins);
		void emitVectorIntGraph(const GcnShaderInstruction& ins);
		void emitVectorMisc(const GcnShaderInstruction& ins);
		void emitScalarProgFlow(const GcnShaderInstruction& ins);
		void emitScalarSync(const GcnShaderInstruction& ins);
		void emitScalarWait(const GcnShaderInstruction& ins);
		void emitScalarCache(const GcnShaderInstruction& ins);
		void emitScalarPrior(const GcnShaderInstruction& ins);
		void emitScalarRegAccess(const GcnShaderInstruction& ins);
		void emitScalarMsg(const GcnShaderInstruction& ins);
		void emitScalarMemRd(const GcnShaderInstruction& ins);
		void emitScalarMemUt(const GcnShaderInstruction& ins);
		void emitVectorMemBufNoFmt(const GcnShaderInstruction& ins);
		void emitVectorMemBufFmt(const GcnShaderInstruction& ins);
		void emitVectorMemBufAtomic(const GcnShaderInstruction& ins);
		void emitVectorMemImgNoSmp(const GcnShaderInstruction& ins);
		void emitVectorMemImgSmp(const GcnShaderInstruction& ins);
		void emitVectorMemImgUt(const GcnShaderInstruction& ins);
		void emitVectorMemL1Cache(const GcnShaderInstruction& ins);
		void emitDsRead(const GcnShaderInstruction& ins);
		void emitDsWrite(const GcnShaderInstruction& ins);
		void emitDsIdxRd(const GcnShaderInstruction& ins);
		void emitDsIdxWr(const GcnShaderInstruction& ins);
		void emitDsIdxWrXchg(const GcnShaderInstruction& ins);
		void emitDsIdxCondXchg(const GcnShaderInstruction& ins);
		void emitDsIdxWrap(const GcnShaderInstruction& ins);
		void emitDsAtomicCommon(const GcnShaderInstruction& ins);
		void emitDsAtomicArith32(const GcnShaderInstruction& ins);
		void emitDsAtomicArith64(const GcnShaderInstruction& ins);
		void emitDsAtomicMinMax32(const GcnShaderInstruction& ins);
		void emitDsAtomicMinMax64(const GcnShaderInstruction& ins);
		void emitDsAtomicCmpSt32(const GcnShaderInstruction& ins);
		void emitDsAtomicCmpSt64(const GcnShaderInstruction& ins);
		void emitDsAtomicLogic32(const GcnShaderInstruction& ins);
		void emitDsAtomicLogic64(const GcnShaderInstruction& ins);
		void emitDsAppendCon(const GcnShaderInstruction& ins);
		void emitDsDataShareUt(const GcnShaderInstruction& ins);
		void emitDsDataShareMisc(const GcnShaderInstruction& ins);
		void emitGdsSync(const GcnShaderInstruction& ins);
		void emitGdsOrdCnt(const GcnShaderInstruction& ins);
		void emitVectorInterpFpCache(const GcnShaderInstruction& ins);
		void emitExp(const GcnShaderInstruction& ins);
		void emitDbgPro(const GcnShaderInstruction& ins);

		/////////////////////////////////////////////////////////
		// Class dispatchers
		void emitExpPosStore(
			uint32_t                regIdx,
			const GcnRegisterValue& value,
			const GcnRegMask&       writeMask);
		void emitExpParamStore(
			uint32_t                regIdx,
			const GcnRegisterValue& value,
			const GcnRegMask&       writeMask);
		void emitExpMrtStore(
			uint32_t                regIdx,
			const GcnRegisterValue& value,
			const GcnRegMask&       writeMask);
		void emitDsSwizzle(
			const GcnShaderInstruction& ins);

		void emitVectorMemBuffer(
			const GcnShaderInstruction& ins);
		void emitCubeCalculate(
			const GcnShaderInstruction& ins);
		void emitLaneReadFirst(
			const GcnShaderInstruction& ins);
		GcnRegisterValue emitCsLaneRead(
			const GcnRegisterValue& slane,
			const GcnRegisterValue& src);
		void emitLaneRead(
			const GcnShaderInstruction& ins);
		GcnRegisterValue emitQueryTextureSize(
			const GcnShaderInstruction& ins);
		GcnRegisterValue emitQueryTextureLevels(
			const GcnShaderInstruction& ins);
		void emitQueryTextureInfo(
			const GcnShaderInstruction& ins);
		void emitQueryTextureLod(
			const GcnShaderInstruction& ins);
		//////////////////////////////////////
		// Common function definition methods
		void emitInit();

		void emitFunctionBegin(
			uint32_t entryPoint,
			uint32_t returnType,
			uint32_t funcType);

		void emitFunctionEnd();

		void emitFunctionLabel();

		void emitMainFunctionBegin();

		/////////////////////////////////
		// Shader initialization methods
		void emitVsInit();
		void emitHsInit();
		void emitDsInit();
		void emitGsInit();
		void emitPsInit();
		void emitCsInit();

		///////////////////////////////
		// Shader finalization methods
		void emitVsFinalize();
		void emitHsFinalize();
		void emitDsFinalize();
		void emitGsFinalize();
		void emitPsFinalize();
		void emitCsFinalize();

		////////////////////////////
		// Input/output preparation
		void emitInitStateRegister();
		void emitInputSetup();
		void emitFetchInput();

		void emitVsInputSetup();
		void emitPsInputSetup();
		void emitCsInputSetup();

		void emitUserDataInit();
		/////////////////////////////////////////////////////
		// Shader interface and metadata declaration methods
		void emitDclGprArray();
		void emitDclGprArray(
			GcnGprArray&       arrayInfo,
			const std::string& name);
		void emitDclInput(
			uint32_t             regIdx,
			GcnInterpolationMode im);
		void emitDclExport();
		void emitDclOutput(
			uint32_t        regIdx,
			GcnExportTarget target);

		void emitDclBuffer(
			const GcnShaderResource& res);
		void emitDclTexture(
			const GcnShaderResource& res);
		void emitDclSampler(
			const GcnShaderResource& res);

		void emitDclStateRegister();
		void emitDclInputSlots();

		void emitDclVertexInput();
		void emitDclPsInput();
		void emitDclThreadGroup();
		void emitDclThreadGroupSharedMemory(uint32_t size);
		void emitDclCrossGroupSharedMemory();

		///////////////////////////////
		// Variable definition methods
		uint32_t emitNewVariable(
			const GcnRegisterInfo& info);

		uint32_t emitNewBuiltinVariable(
			const GcnRegisterInfo& info,
			spv::BuiltIn           builtIn,
			const char*            name);

		//////////////////////////////////////////
		// System value load methods (per shader)
		GcnRegisterValue emitCommonSystemValueLoad(
			GcnSystemValue sv,
			GcnRegMask     mask);

		GcnRegisterValue emitVsSystemValueLoad(
			GcnSystemValue sv,
			GcnRegMask     mask);

		GcnRegisterValue emitPsSystemValueLoad(
			GcnSystemValue sv,
			GcnRegMask     mask);

		GcnRegisterValue emitCsSystemValueLoad(
			GcnSystemValue sv,
			GcnRegMask     mask);
		///////////////////////////////////////////
		// System value store methods (per shader)
		void emitVsSystemValueStore(
			GcnSystemValue          sv,
			GcnRegMask              mask,
			const GcnRegisterValue& value);

		void emitPsSystemValueStore(
			GcnSystemValue          sv,
			GcnRegMask              mask,
			const GcnRegisterValue& value);
    

		///////////////////////////////
		// SGPR/VGPR load/store methods
		template <bool IsVgpr>
		GcnRegisterPointer emitGetGprPtr(
			uint32_t indexId);
		template <bool IsVgpr>
		GcnRegisterPointer emitGetGprPtr(
			const GcnInstOperand& reg);

		template <bool IsVgpr>
		GcnRegisterValue emitGprLoad(
			const GcnInstOperand& reg);
		GcnRegisterValue emitVgprLoad(
			const GcnInstOperand& reg);
		GcnRegisterValue emitSgprLoad(
			const GcnInstOperand& reg);

		template <bool IsVgpr>
		GcnRegisterValue emitGprLoad(uint32_t indexId);
		GcnRegisterValue emitVgprLoad(uint32_t indexId);
		GcnRegisterValue emitSgprLoad(uint32_t indexId);

		template <bool IsVgpr>
		GcnRegisterValue emitGprArrayLoad(
			const GcnInstOperand& start,
			const GcnRegMask&     mask);
		GcnRegisterValue emitVgprArrayLoad(
			const GcnInstOperand& start,
			const GcnRegMask&     mask);
		GcnRegisterValue emitSgprArrayLoad(
			const GcnInstOperand& start,
			const GcnRegMask&     mask);

		template <bool IsVgpr>
		void emitGprStore(
			const GcnInstOperand&   reg,
			const GcnRegisterValue& value);
		void emitVgprStore(
			const GcnInstOperand&   reg,
			const GcnRegisterValue& value);
		void emitSgprStore(
			const GcnInstOperand&   reg,
			const GcnRegisterValue& value);

		template <bool IsVgpr>
		void emitGprStore(
			uint32_t                indexId,
			const GcnRegisterValue& value);
		void emitVgprStore(
			uint32_t                indexId,
			const GcnRegisterValue& value);
		void emitSgprStore(
			uint32_t                indexId,
			const GcnRegisterValue& value);

		template <bool IsVgpr>
		void emitGprArrayStore(
			const GcnInstOperand&   start,
			const GcnRegisterValue& value,
			const GcnRegMask&       mask);
		void emitVgprArrayStore(
			const GcnInstOperand&   start,
			const GcnRegisterValue& value,
			const GcnRegMask&       mask);
		void emitSgprArrayStore(
			const GcnInstOperand&   start,
			const GcnRegisterValue& value,
			const GcnRegMask&       mask);

		//////////////////////////////
		// Operand load/store methods
		GcnRegisterValue emitValueLoad(
			GcnRegisterPointer ptr);

		void emitValueStore(
			GcnRegisterPointer ptr,
			GcnRegisterValue   value,
			GcnRegMask         writeMask);

		GcnRegisterValuePair emitRegisterLoad(
			const GcnInstOperand& reg);

		void emitRegisterStore(
			const GcnInstOperand&       reg,
			const GcnRegisterValuePair& value);

		GcnRegisterPointer emitVectorAccess(
			GcnRegisterPointer pointer,
			spv::StorageClass  sclass,
			const GcnRegMask&  mask);

		GcnRegisterValue emitIndexLoad(
			const GcnRegIndex& index);

		void emitTextureSample(
			const GcnShaderInstruction& ins);

		void emitStorageImageLoad(
			const GcnShaderInstruction& ins);

		GcnRegisterValue emitCalcTexCoord(
			GcnRegisterValue    coordVector,
			const GcnImageInfo& imageInfo);

		GcnRegisterValue emitLoadTexCoord(
			const GcnShaderInstruction& ins);

		GcnRegisterValue emitLoadTexOffset(
			const GcnShaderInstruction& ins);

		GcnRegisterValue emitRecoverCubeCoord(
			const GcnRegisterValue& coord);

		GcnRegisterValue emitLoadAddrComponent(
			GcnImageAddrComponent       component,
			const GcnShaderInstruction& ins);

		GcnRegisterValue emitCalcBufferAddress(
			const GcnShaderInstruction& ins);

		///////////////////////////////
		// Resource load/store methods

		std::vector<GcnRegisterPointer>
		emitUniformBufferAccess(
			uint32_t bufferId,
			uint32_t baseId,
			uint32_t count);

		std::vector<GcnRegisterPointer>
		emitStorageBufferAccess(
			uint32_t bufferId,
			uint32_t baseId,
			uint32_t count);

		std::array<GcnRegisterPointer, 4>
		     emitDsAccess(const GcnShaderInstruction& ins);

		void emitRawBufferLoad(
			const GcnShaderInstruction& ins);

		void emitConstantBufferLoad(
			const GcnRegIndex&    index,
			const GcnInstOperand& dst,
			uint32_t              count);

		std::vector<GcnRegisterPointer>
		emitGetBufferComponentPtr(
			const GcnShaderInstruction& ins,
			bool                        isFormat);

		void emitBufferLoadStoreNoFmt(
			const GcnShaderInstruction& ins,
			bool                        isLoad);

		void emitBufferLoadStoreFmt(
			const GcnShaderInstruction& ins,
			bool                        isLoad);

		///////////////////////////////////////
		// Image register manipulation methods
		uint32_t emitLoadSampledImage(
			const GcnTexture& textureResource,
			const GcnSampler& samplerResource,
			bool              isDepthCompare);

		////////////////////////////////////////////////
		// Constant building methods. These are used to
		// generate constant vectors that store the same
		// value in each component.
		GcnRegisterValue emitBuildConstVecf32(
			float              x,
			float              y,
			float              z,
			float              w,
			const GcnRegMask& writeMask);

		GcnRegisterValue emitBuildConstVecu32(
			uint32_t           x,
			uint32_t           y,
			uint32_t           z,
			uint32_t           w,
			const GcnRegMask& writeMask);

		GcnRegisterValue emitBuildConstVeci32(
			int32_t            x,
			int32_t            y,
			int32_t            z,
			int32_t            w,
			const GcnRegMask& writeMask);

		GcnRegisterValue emitBuildConstVecf64(
			double             xy,
			double             zw,
			const GcnRegMask& writeMask);

		GcnRegisterValue emitBuildConstValue(
			size_t        value,
			GcnScalarType type);

		GcnRegisterValuePair emitBuildLiteralConst(
			const GcnInstOperand& reg);
		GcnRegisterValuePair emitBuildInlineConst(
			const GcnInstOperand& reg);

		/////////////////////////////////////////
		// Generic register manipulation methods
		GcnRegisterValue emitRegisterBitcast(
			GcnRegisterValue srcValue,
			GcnScalarType    dstType);

		GcnRegisterValuePair emitRegisterBitcast(
			GcnRegisterValuePair srcValue,
			GcnScalarType        dstType);

		GcnRegisterValue emitRegisterSwizzle(
			GcnRegisterValue value,
			GcnRegSwizzle    swizzle,
			GcnRegMask       writeMask);

		GcnRegisterValue emitRegisterExtract(
			GcnRegisterValue value,
			GcnRegMask       mask);

		GcnRegisterValue emitRegisterInsert(
			GcnRegisterValue dstValue,
			GcnRegisterValue srcValue,
			GcnRegMask       srcMask);

		GcnRegisterValue emitRegisterConcat(
			GcnRegisterValue value1,
			GcnRegisterValue value2);

		GcnRegisterValue emitRegisterExtend(
			GcnRegisterValue value,
			uint32_t         size);

		GcnRegisterValue emitRegisterAbsolute(
			GcnRegisterValue value);

		GcnRegisterValue emitRegisterNegate(
			GcnRegisterValue value);

		GcnRegisterValue emitRegisterZeroTest(
			GcnRegisterValue value,
			GcnZeroTest      test);

		GcnRegisterValue emitRegisterMaskBits(
			GcnRegisterValue value,
			uint32_t         mask);

		GcnRegisterValue emitInputModifiers(
			GcnRegisterValue  value,
			GcnInputModifiers modifiers);

		GcnRegisterValue emitOutputModifiers(
			GcnRegisterValue   value,
			GcnOutputModifiers modifiers);

		GcnRegisterValue emitPackHalf2x16(
			GcnRegisterValuePair src);

		GcnRegisterValuePair emitUnpackHalf2x16(
			GcnRegisterValue src);

		GcnRegisterValue emitWholeQuadMode(
			GcnRegisterValue src);

		void emitUpdateScc(
			GcnRegisterValuePair& dst,
			GcnScalarType         dstType);

		///////////////////////////
		// Debug methods
		template <typename... Args>
		void emitDebugPrintf(
			const std::string& format, Args... args)
		{
			// Copy vk_layer_settings.txt from Misc folder to your
			// Current Working Directory or use vkconfig.exe from SDK
			// to enable shader debug printf.
			// Note that normally we need to specify big enough
			// buffer size and limit the number of messages by conditionally using invocation ID
			//
			// Format for specifier is "%"precision <d, i, o, u, x, X, a, A, e, E, f, F, g, G, or ul>
			// Format for vector specifier is "%"precision"v" [2, 3, or 4] [specifiers list above]
			// 
			// To conditionally print something,
			// copy and edit the following code:
			// 
			//if (m_header->key().name() == "SHDR_AF20AC1F702451D8" && m_programCounter == 0x70)
			//{
			//	auto     invId      = emitCommonSystemValueLoad(GcnSystemValue::SubgroupInvocationID, 0);
			//	auto     condition  = m_module.opIEqual(m_module.defBoolType(), invId.id, m_module.constu32(1));
			//	uint32_t labelBegin = m_module.allocateId();
			//	uint32_t labelEnd   = m_module.allocateId();
			//	m_module.opSelectionMerge(labelEnd, spv::SelectionControlMaskNone);
			//	m_module.opBranchConditional(condition, labelBegin, labelEnd);
			//	m_module.opLabel(labelBegin);
			//	emitDebugPrintf("id %X\n", invId.id);
			//	m_module.opBranch(labelEnd);
			//	m_module.opLabel(labelEnd);
			//}

#ifdef GCN_SHADER_DEBUG_PRINTF
			const int count = sizeof...(args);

			std::array<uint32_t, count> arguments = { args... };
			m_module.opDebugPrintf(format.c_str(),
								   arguments.size(),
								   arguments.data());
#endif  // GCN_SHADER_DEBUG_PRINTF
		}

		
		///////////////////////////
		// Control flow methods
		GcnCfgBlock* cfgFindBlock(
			const std::initializer_list<GcnCfgBlockType>& types);

		bool needBreakLoop(
			const GcnToken& token);
	
		///////////////////////////
		// Type definition methods
		uint32_t getScalarTypeId(
			GcnScalarType type);

		uint32_t getVectorTypeId(
			const GcnVectorType& type);

		uint32_t getArrayTypeId(
			const GcnArrayType& type);

		uint32_t getPointerTypeId(
			const GcnRegisterInfo& type);

		uint32_t getPerVertexBlockId();


		///////////////////////////
		//
		bool isDoubleType(
			GcnScalarType type) const;

		bool isFloatType(
			GcnScalarType type) const;

		GcnScalarType getHalfType(
			GcnScalarType type) const;

		GcnScalarType getDestinationType(
			GcnScalarType type) const;

		uint32_t getUserSgprCount() const;

		bool hasFetchShader() const;

		std::pair<const VertexInputSemantic*, uint32_t>
			getSemanticTable() const;

		const std::array<GcnTextureMeta, 128>&
			getTextureMetaTable();

		GcnBufferInfo getBufferType(
			const GcnInstOperand& reg);

		GcnImageInfo getImageType(
			Gnm::TextureType textureType,
			bool             isStorage,
			bool             isDepth) const;

		GcnImageInfo getImageInfo(
			const GcnShaderInstruction& ins) const;

		GcnVectorType getInputRegType(
			uint32_t regIdx) const;

		GcnVectorType getOutputRegType(
			uint32_t paramIdx) const;

		uint32_t getTexSizeDim(
			const GcnImageInfo& imageType) const;

		uint32_t getTexLayerDim(
			const GcnImageInfo& imageType) const;

		uint32_t getTexCoordDim(
			const GcnImageInfo& imageType) const;

		GcnBufferFormat getBufferFormat(
			Gnm::BufferFormat      dfmt,
			Gnm::BufferChannelType nfmt);

		uint32_t calcAddrComponentIndex(
			GcnImageAddrComponent       component,
			spv::Dim                    dim,
			const GcnShaderInstruction& ins);

		void mapNonEudResource();

	private:
		GcnModuleInfo          m_moduleInfo;
		GcnProgramInfo         m_programInfo;
		const GcnHeader*       m_header;
		GcnShaderMeta          m_meta;
		const GcnAnalysisInfo* m_analysis;
		SpirvModule            m_module;

		// Hardware state registers.
		GcnStateRegisters m_state;
		///////////////////////////////////////////////////
		// Entry point description - we'll need to declare
		// the function ID and all input/output variables.
		uint32_t              m_entryPointId = 0;
		std::vector<uint32_t> m_entryPointInterfaces;

		///////////////////////////////////////////////////
		// Shader input/output interfaces
		std::array<
			GcnRegisterPointer, 
			GcnMaxInterfaceRegs> m_inputs;
		// Export params
		std::array<
			GcnRegisterPointer,
			GcnMaxInterfaceRegs> m_params;
		// Export mrts
		std::array<
			GcnRegisterPointer,
			GcnMaxInterfaceRegs> m_mrts;

		///////////////////////////////////////////////////
		// SGPR/VGRP container
		// Some instructions use dynamic index,
		// so we need to declare gprs in array.
		GcnGprArray m_sArray;
		GcnGprArray m_vArray;

		//////////////////////////////////////////////////////
		// Shader resource variables. These provide access to
		// buffers, samplers and textures.
		std::array<GcnBuffer, GcnMaxResourceReg>  m_buffersDcl;
		std::array<GcnSampler, GcnMaxResourceReg> m_samplersDcl;
		std::array<GcnTexture, GcnMaxResourceReg> m_texturesDcl;

		std::array<GcnBuffer, GcnMaxResourceReg>  m_buffers;
		std::array<GcnSampler, GcnMaxResourceReg> m_samplers;
		std::array<GcnTexture, GcnMaxResourceReg> m_textures;

		//////////////////////////////////////////////
		// Function/Block state tracking. Required in order
		// to properly end functions and blocks in some cases.
		bool m_insideFunction = false;
		///////////////////////////////////////////////////////////
		// An array stores up to 16 user data registers.
		uint32_t m_vUserDataArray = 0;

		////////////////////////////////////////////////////
		// Per-vertex input and output blocks. Depending on
		// the shader stage, these may be declared as arrays.
		uint32_t m_perVertexIn  = 0;
		uint32_t m_perVertexOut = 0;

		uint32_t m_primitiveIdIn = 0;

		////////////////////////////////////////////////////
		// LDS
		uint32_t m_lds = 0;

		///////////////////////////////////////////////////////
		// Resource slot description for the shader.
		std::vector<vlt::VltResourceSlot> m_resourceSlots;
		////////////////////////////////////////////
		// Inter-stage shader interface slots. Also
		// covers vertex input and fragment output.
		vlt::VltInterfaceSlots m_interfaceSlots;
		//////////////////////////////////////////////////
		// Immediate constant buffer. If defined, this is
		// an array of four-component uint32 vectors.
		uint32_t                m_immConstBuf = 0;
		vlt::VltShaderConstData m_immConstData;
		///////////////////////////////////////////////
		// Control flow scope stack. Stores labels for
		// currently active if-else blocks and loops.
		std::vector<GcnCfgBlock> m_controlFlowStack;
		std::vector<uint32_t>    m_blockTerminators;
		std::unordered_map<
			const GcnToken*,
			GcnRegisterPointer>  m_tokenVariables;

		///////////////////////////////////
		// Shader-specific data structures
		GcnCompilerCommonPart m_common;
		GcnCompilerVsPart     m_vs;
		GcnCompilerPsPart     m_ps;
		GcnCompilerCsPart     m_cs;
	};




}  // namespace sce::gcn