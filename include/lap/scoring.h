#pragma once
#include "model.h"
namespace lap {
TelemetrySummary SummarizeTelemetry(const std::vector<TelemetrySample>& samples);
void AssessStressStage(StressStageResult& stage);
AuditDecision BuildAuditDecision(const AuditReport& report);
const wchar_t* ConfidenceText(Confidence c);
const wchar_t* VerdictText(TestVerdict v);
}
