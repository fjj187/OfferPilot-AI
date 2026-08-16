#include "Controller/interview_controller.hpp"

inline void to_json(nlohmann::json& j, const PracticePlanSnapshot& p) {
    j = nlohmann::json{
        {"weaknessTag", p.weaknessTag},
        {"questionType", p.questionType},
        {"difficulty", p.difficulty},
        {"zone", p.zone}
    };
}

inline void from_json(const nlohmann::json& j, PracticePlanSnapshot& p) {
    j.at("weaknessTag").get_to(p.weaknessTag);
    j.at("questionType").get_to(p.questionType);
    j.at("difficulty").get_to(p.difficulty);
    j.at("zone").get_to(p.zone);
}

inline void to_json(nlohmann::json& j, const ReportQuestionReview& r) {
    j = nlohmann::json{
        {"questionId", r.questionId},
        {"questionTitle", r.questionTitle},
        {"userAnswer", r.userAnswer},
        {"referenceAnswer", r.referenceAnswer ? nlohmann::json(*r.referenceAnswer) : nlohmann::json(nullptr)},
        {"aiFeedback", r.aiFeedback ? nlohmann::json(*r.aiFeedback) : nlohmann::json(nullptr)}
    };
}

inline void from_json(const nlohmann::json& j, ReportQuestionReview& r) {
    r.questionId = j.value("questionId", "");
    r.questionTitle = j.value("questionTitle", "");
    r.userAnswer = j.value("userAnswer", "");
    if (j.contains("referenceAnswer") && !j.at("referenceAnswer").is_null()) {
        r.referenceAnswer = j.at("referenceAnswer").get<std::string>();
    }
    if (j.contains("aiFeedback") && !j.at("aiFeedback").is_null()) {
        r.aiFeedback = j.at("aiFeedback").get<std::string>();
    }
}

inline void to_json(nlohmann::json& j, const InterviewReportEntity& r) {
    j = nlohmann::json{
        {"id", r.id},
        {"sessionId", r.sessionId},
        {"modelId", r.modelId ? nlohmann::json(*r.modelId) : nlohmann::json(nullptr)},
        {"threadId", r.threadId ? nlohmann::json(*r.threadId) : nlohmann::json(nullptr)},
        {"topic", r.topic},
        {"source", r.source},
        {"sourceDocumentId", r.sourceDocumentId ? nlohmann::json(*r.sourceDocumentId) : nlohmann::json(nullptr)},
        {"sourceDocumentName", r.sourceDocumentName ? nlohmann::json(*r.sourceDocumentName) : nlohmann::json(nullptr)},
        {"sourceDocumentExcerpt", r.sourceDocumentExcerpt ? nlohmann::json(*r.sourceDocumentExcerpt) : nlohmann::json(nullptr)},
        {"questionTitle", r.questionTitle ? nlohmann::json(*r.questionTitle) : nlohmann::json(nullptr)},
        {"summaryHeadline", r.summaryHeadline},
        {"summaryBody", r.summaryBody},
        {"weaknessTags", r.weaknessTags},
        {"primaryWeakness", r.primaryWeakness ? nlohmann::json(*r.primaryWeakness) : nlohmann::json(nullptr)},
        {"weaknessFocusAreas", r.weaknessFocusAreas ? nlohmann::json(*r.weaknessFocusAreas) : nlohmann::json(nullptr)},
        {"answeredCount", r.answeredCount},
        {"totalCount", r.totalCount},
        {"answerSnapshot", r.answerSnapshot ? nlohmann::json(*r.answerSnapshot) : nlohmann::json(nullptr)},
        {"questionReviews", r.questionReviews ? nlohmann::json(*r.questionReviews) : nlohmann::json(nullptr)},
        {"suggestedFocus", r.suggestedFocus ? nlohmann::json(*r.suggestedFocus) : nlohmann::json(nullptr)},
        {"practicePlan", r.practicePlan ? nlohmann::json(*r.practicePlan) : nlohmann::json(nullptr)},
        {"createdAt", r.createdAt},
        {"updatedAt", r.updatedAt}
    };
}

inline void from_json(const nlohmann::json& j, InterviewReportEntity& r) {
    j.at("id").get_to(r.id);
    j.at("sessionId").get_to(r.sessionId);
    if (j.contains("modelId") && !j.at("modelId").is_null()) {
        r.modelId = j.at("modelId").get<std::string>();
    }
    if (j.contains("threadId") && !j.at("threadId").is_null()) {
        r.threadId = j.at("threadId").get<std::string>();
    }
    j.at("topic").get_to(r.topic);
    j.at("source").get_to(r.source);
    if (j.contains("sourceDocumentId") && !j.at("sourceDocumentId").is_null()) {
        r.sourceDocumentId = j.at("sourceDocumentId").get<std::string>();
    }
    if (j.contains("sourceDocumentName") && !j.at("sourceDocumentName").is_null()) {
        r.sourceDocumentName = j.at("sourceDocumentName").get<std::string>();
    }
    if (j.contains("sourceDocumentExcerpt") && !j.at("sourceDocumentExcerpt").is_null()) {
        r.sourceDocumentExcerpt = j.at("sourceDocumentExcerpt").get<std::string>();
    }
    if (j.contains("questionTitle") && !j.at("questionTitle").is_null()) {
        r.questionTitle = j.at("questionTitle").get<std::string>();
    }
    j.at("summaryHeadline").get_to(r.summaryHeadline);
    j.at("summaryBody").get_to(r.summaryBody);
    if (j.contains("weaknessTags") && j.at("weaknessTags").is_array()) {
        r.weaknessTags = j.at("weaknessTags").get<std::vector<std::string>>();
    }
    if (j.contains("primaryWeakness") && !j.at("primaryWeakness").is_null()) {
        r.primaryWeakness = j.at("primaryWeakness").get<std::string>();
    }
    if (j.contains("weaknessFocusAreas") && !j.at("weaknessFocusAreas").is_null()) {
        r.weaknessFocusAreas = j.at("weaknessFocusAreas").get<std::vector<std::string>>();
    }
    j.at("answeredCount").get_to(r.answeredCount);
    j.at("totalCount").get_to(r.totalCount);
    if (j.contains("answerSnapshot") && !j.at("answerSnapshot").is_null()) {
        r.answerSnapshot = j.at("answerSnapshot").get<std::vector<std::string>>();
    }
    if (j.contains("questionReviews") && !j.at("questionReviews").is_null()) {
        r.questionReviews = j.at("questionReviews").get<std::vector<ReportQuestionReview>>();
    }
    if (j.contains("suggestedFocus") && !j.at("suggestedFocus").is_null()) {
        r.suggestedFocus = j.at("suggestedFocus").get<std::vector<std::string>>();
    }
    if (j.contains("practicePlan") && !j.at("practicePlan").is_null()) {
        r.practicePlan = j.at("practicePlan").get<PracticePlanSnapshot>();
    }
    j.at("createdAt").get_to(r.createdAt);
    j.at("updatedAt").get_to(r.updatedAt);
}

inline void to_json(nlohmann::json& j, const InterviewReportSummary& r) {
    j = nlohmann::json{
        {"id", r.id},
        {"sessionId", r.sessionId},
        {"threadId", r.threadId ? nlohmann::json(*r.threadId) : nlohmann::json(nullptr)},
        {"topic", r.topic},
        {"questionTitle", r.questionTitle ? nlohmann::json(*r.questionTitle) : nlohmann::json(nullptr)},
        {"summaryHeadline", r.summaryHeadline},
        {"answeredCount", r.answeredCount},
        {"totalCount", r.totalCount},
        {"weaknessTags", r.weaknessTags},
        {"createdAt", r.createdAt},
        {"updatedAt", r.updatedAt}
    };
}

static std::optional<InterviewFeedbackStyle> parseFeedbackStyle(const std::string& s) {
    if (s == "followup") return InterviewFeedbackStyle::Followup;
    if (s == "corrective") return InterviewFeedbackStyle::Corrective;
    if (s == "guided") return InterviewFeedbackStyle::Guided;
    return std::nullopt;
}

static std::optional<InterviewMessageFormat> parseFormat(const std::string& s) {
    if (s == "plain") return InterviewMessageFormat::Plain;
    if (s == "markdown") return InterviewMessageFormat::Markdown;
    return std::nullopt;
}

static std::optional<std::string> feedbackStyleToString(const std::optional<InterviewFeedbackStyle>& style) {
    if (!style.has_value()) return std::nullopt;
    switch (*style) {
        case InterviewFeedbackStyle::Followup: return "followup";
        case InterviewFeedbackStyle::Corrective: return "corrective";
        case InterviewFeedbackStyle::Guided: return "guided";
    }
    return std::nullopt;
}

InterviewStreamRequest InterviewController::parseStreamRequest(const nlohmann::json& body) const {
    InterviewStreamRequest request;
    request.sessionId = body.value("sessionId", "");
    request.messageId = body.value("messageId", "");
    request.threadId = body.value("threadId", "");
    request.topic = body.value("topic", "");
    request.topicLabel = body.value("topicLabel", "");
    request.prompt = body.value("prompt", "");
    request.questionTitle = body.value("questionTitle", "");
    request.questionPrompt = body.value("questionPrompt", "");
    request.answer = body.value("answer", "");

    if (body.contains("sourceContext") && body["sourceContext"].is_string())
        request.sourceContext = body["sourceContext"].get<std::string>();
    if (body.contains("sourceDocumentName") && body["sourceDocumentName"].is_string())
        request.sourceDocumentName = body["sourceDocumentName"].get<std::string>();
    if (body.contains("sourceDocumentSummary") && body["sourceDocumentSummary"].is_string())
        request.sourceDocumentSummary = body["sourceDocumentSummary"].get<std::string>();
    if (body.contains("sourceDocumentTags") && body["sourceDocumentTags"].is_array()) {
        std::vector<std::string> tags;
        for (const auto& item : body["sourceDocumentTags"]) {
            if (item.is_string()) tags.push_back(item.get<std::string>());
        }
        request.sourceDocumentTags = std::move(tags);
    }
    if (body.contains("sourceDocumentExcerpt") && body["sourceDocumentExcerpt"].is_string())
        request.sourceDocumentExcerpt = body["sourceDocumentExcerpt"].get<std::string>();

    if (body.contains("options") && body["options"].is_object()) {
        const auto& options = body["options"];

        if (options.contains("feedbackStyle") && options["feedbackStyle"].is_string())
            request.options.feedbackStyle = parseFeedbackStyle(options["feedbackStyle"].get<std::string>());
        if (options.contains("format") && options["format"].is_string())
            request.options.format = parseFormat(options["format"].get<std::string>());
        if (options.contains("questionIndex") && options["questionIndex"].is_number_integer())
            request.options.questionIndex = options["questionIndex"].get<int>();
        if (options.contains("questionCount") && options["questionCount"].is_number_integer())
            request.options.questionCount = options["questionCount"].get<int>();
        if (options.contains("unknownAnswerStreak") && options["unknownAnswerStreak"].is_number_integer())
            request.options.unknownAnswerStreak = options["unknownAnswerStreak"].get<int>();
        if (options.contains("forceRevealReferenceAnswer") && options["forceRevealReferenceAnswer"].is_boolean())
            request.options.forceRevealReferenceAnswer = options["forceRevealReferenceAnswer"].get<bool>();
        if (options.contains("referenceAnswerHint") && options["referenceAnswerHint"].is_string())
            request.options.referenceAnswerHint = options["referenceAnswerHint"].get<std::string>();
    }

    return request;
}

std::string InterviewController::serializeSseEvent(const std::string& eventName, const std::string& data) {
    return "event: " + eventName + "\ndata: " + data + "\n\n";
}

void InterviewController::generateReport(const httplib::Request& req, httplib::Response& res) {
    try {
        const auto body = nlohmann::json::parse(req.body);

        GenerateReportRequest request;
        request.sessionId = body.value("sessionId", "");
        request.topic = body.contains("topic") ? std::optional<std::string>(body.value("topic", "")) : std::nullopt;
        request.source = body.contains("source") ? std::optional<std::string>(body.value("source", "")) : std::nullopt;
        request.sourceDocumentId = body.contains("sourceDocumentId") ? std::optional<std::string>(body.value("sourceDocumentId", "")) : std::nullopt;
        request.sourceDocumentName = body.contains("sourceDocumentName") ? std::optional<std::string>(body.value("sourceDocumentName", "")) : std::nullopt;
        request.modelId = body.contains("modelId") ? std::optional<std::string>(body.value("modelId", "")) : std::nullopt;
        request.summaryBody = body.contains("summaryBody") ? std::optional<std::string>(body.value("summaryBody", "")) : std::nullopt;
        request.sourceDocumentExcerpt = body.contains("sourceDocumentExcerpt") ? std::optional<std::string>(body.value("sourceDocumentExcerpt", "")) : std::nullopt;

        if (body.contains("answeredCount") && body["answeredCount"].is_number_integer()) {
            request.answeredCount = body["answeredCount"].get<int>();
        }
        if (body.contains("totalCount") && body["totalCount"].is_number_integer()) {
            request.totalCount = body["totalCount"].get<int>();
        }
        if (body.contains("primaryWeakness") && body["primaryWeakness"].is_string()) {
            request.primaryWeakness = body["primaryWeakness"].get<std::string>();
        }
        if (body.contains("weaknessTags") && body["weaknessTags"].is_array()) {
            std::vector<std::string> tags;
            for (const auto& item : body["weaknessTags"]) {
                if (item.is_string()) tags.push_back(item.get<std::string>());
            }
            request.weaknessTags = std::move(tags);
        }
        if (body.contains("weaknessFocusAreas") && body["weaknessFocusAreas"].is_array()) {
            std::vector<std::string> areas;
            for (const auto& item : body["weaknessFocusAreas"]) {
                if (item.is_string()) areas.push_back(item.get<std::string>());
            }
            request.weaknessFocusAreas = std::move(areas);
        }
        if (body.contains("suggestedFocus") && body["suggestedFocus"].is_array()) {
            std::vector<std::string> focus;
            for (const auto& item : body["suggestedFocus"]) {
                if (item.is_string()) focus.push_back(item.get<std::string>());
            }
            request.suggestedFocus = std::move(focus);
        }
        if (body.contains("questionReviews") && body["questionReviews"].is_array()) {
            std::vector<ReportQuestionReview> reviews;
            for (const auto& item : body["questionReviews"]) {
                if (!item.is_object()) continue;
                ReportQuestionReview review;
                review.questionId = item.value("questionId", "");
                review.questionTitle = item.value("questionTitle", "");
                review.userAnswer = item.value("userAnswer", "");
                if (item.contains("referenceAnswer") && item["referenceAnswer"].is_string()) {
                    review.referenceAnswer = item["referenceAnswer"].get<std::string>();
                }
                if (item.contains("aiFeedback") && item["aiFeedback"].is_string()) {
                    review.aiFeedback = item["aiFeedback"].get<std::string>();
                }
                reviews.push_back(std::move(review));
            }
            request.questionReviews = std::move(reviews);
        }

        if (request.sessionId.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"sessionId is required"})", "application/json");
            return;
        }

        const auto result = m_reportService.generateReport(request);

        nlohmann::json j;
        j["report"] = result.report;
        j["created"] = result.created;
        j["aiUsed"] = result.aiUsed;
        j["fallbackReason"] = result.fallbackReason ? nlohmann::json(*result.fallbackReason) : nlohmann::json(nullptr);
        res.set_content(j.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(std::string("{\"success\":false,\"error\":\"") + e.what() + "\"}", "application/json");
    }
}

void InterviewController::getReport(const httplib::Request& req, httplib::Response& res) {
    try {
        const auto sessionId = req.path_params.at("sessionId");
        if (sessionId.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"sessionId is required"})", "application/json");
            return;
        }

        const auto report = m_reportService.getReportBySessionId(sessionId);
        if (!report.has_value()) {
            res.status = 404;
            res.set_content(R"({"success":false,"error":"Report not found"})", "application/json");
            return;
        }

        nlohmann::json j;
        j["report"] = *report;
        res.set_content(j.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(std::string("{\"success\":false,\"error\":\"") + e.what() + "\"}", "application/json");
    }
}

void InterviewController::listReports(const httplib::Request&, httplib::Response& res) {
    try {
        const auto reports = m_reportService.listReports();

        nlohmann::json j;
        j["reports"] = nlohmann::json::array();
        for (const auto& report : reports) {
            j["reports"].push_back(report);
        }

        res.set_content(j.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(std::string("{\"success\":false,\"error\":\"") + e.what() + "\"}", "application/json");
    }
}

void InterviewController::createSession(const httplib::Request&, httplib::Response& res) {
    res.status = 501;
    res.set_content("{\"success\":false,\"error\":\"Not implemented\"}", "application/json");
}

void InterviewController::sendMessage(const httplib::Request&, httplib::Response& res) {
    res.status = 501;
    res.set_content("{\"success\":false,\"error\":\"Not implemented\"}", "application/json");
}

void InterviewController::listSessions(const httplib::Request&, httplib::Response& res) {
    const auto sessions = m_service.listSessions();
    nlohmann::json j;
    j["sessions"] = nlohmann::json::array();

    for (const auto& s : sessions) {
        nlohmann::json item;
        item["sessionId"] = s.sessionId;
        item["threadId"] = s.threadId;
        item["topic"] = s.topic;
        item["questionTitle"] = s.questionTitle;
        item["feedbackStyle"] = feedbackStyleToString(s.feedbackStyle);
        item["messageCount"] = s.messageCount;
        item["latestUserMessage"] = s.latestUserMessage;
        item["latestAssistantMessage"] = s.latestAssistantMessage;
        item["createdAt"] = s.createdAt;
        item["updatedAt"] = s.updatedAt;
        j["sessions"].push_back(std::move(item));
    }

    res.set_content(j.dump(), "application/json");
}

void InterviewController::getSession(const httplib::Request& req, httplib::Response& res) {
    const auto sessionId = req.path_params.at("sessionId");
    const auto threadId = req.path_params.at("threadId");

    const auto detail = m_service.getSession(sessionId, threadId);
    if (!detail.has_value()) {
        res.status = 404;
        res.set_content(R"({"success":false,"error":"Session not found"})", "application/json");
        return;
    }

    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : detail->messages) {
        messages.push_back({
            {"role", msg.role == InterviewMessageRole::User ? "user" : "assistant"},
            {"content", msg.content},
            {"createdAt", msg.createdAt}
        });
    }

    nlohmann::json session;
    session["sessionId"] = detail->sessionId;
    session["threadId"] = detail->threadId;
    session["topic"] = detail->topic;
    session["questionTitle"] = detail->questionTitle;
    session["feedbackStyle"] = feedbackStyleToString(detail->feedbackStyle);
    session["messageCount"] = detail->messageCount;
    session["latestUserMessage"] = detail->latestUserMessage;
    session["latestAssistantMessage"] = detail->latestAssistantMessage;
    session["createdAt"] = detail->createdAt;
    session["updatedAt"] = detail->updatedAt;
    session["messages"] = std::move(messages);

    nlohmann::json j;
    j["session"] = std::move(session);
    res.set_content(j.dump(), "application/json");
}

void InterviewController::clearHistory(const httplib::Request&, httplib::Response& res) {
    m_service.clearHistory();
    res.set_content(R"({"ok":true})", "application/json");
}
