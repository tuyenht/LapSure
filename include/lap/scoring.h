#pragma once
#include "decision_context.h"
namespace lap {
TelemetrySummary SummarizeTelemetry(const std::vector<TelemetrySample>& samples);
void AssessStressStage(StressStageResult& stage);
AuditDecision BuildAuditDecision(const AuditReport& report);
AuditDecision BuildAuditDecision(const AuditReport& report, const DecisionContext& context);
std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report);
std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report, const DecisionContext& context);
const wchar_t* ConfidenceText(Confidence c);
const wchar_t* VerdictText(TestVerdict v);
}