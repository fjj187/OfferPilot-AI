#include "Controller/InterviewStreamController.hpp"

#include <utility>

InterviewStreamController::InterviewStreamController(InterviewStreamManager& manager)
    : m_manager(manager) {}

InterviewStreamRequest InterviewStreamController::parseStreamRequest(const nlohmann::json& body) const {
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

    if (body.contains("sourceContext") && body["sourceContext"].is_string()) {
        request.sourceContext = body["sourceContext"].get<std::string>();
    }
    if (body.contains("sourceDocumentName") && body["sourceDocumentName"].is_string()) {
        request.sourceDocumentName = body["sourceDocumentName"].get<std::string>();
    }
    if (body.contains("sourceDocumentSummary") && body["sourceDocumentSummary"].is_string()) {
        request.sourceDocumentSummary = body["sourceDocumentSummary"].get<std::string>();
    }
    if (body.contains("sourceDocumentTags") && body["sourceDocumentTags"].is_array()) {
        std::vector<std::string> tags;
        for (const auto& item : body["sourceDocumentTags"]) {
            if (item.is_string()) {
                tags.push_back(item.get<std::string>());
            }
        }
        request.sourceDocumentTags = std::move(tags);
    }
    if (body.contains("sourceDocumentExcerpt") && body["sourceDocumentExcerpt"].is_string()) {
        request.sourceDocumentExcerpt = body["sourceDocumentExcerpt"].get<std::string>();
    }

    if (body.contains("options") && body["options"].is_object()) {
        const auto& options = body["options"];

        if (options.contains("feedbackStyle") && options["feedbackStyle"].is_string()) {
            const auto s = options["feedbackStyle"].get<std::string>();
            if (s == "followup") request.options.feedbackStyle = InterviewFeedbackStyle::Followup;
            else if (s == "corrective") request.options.feedbackStyle = InterviewFeedbackStyle::Corrective;
            else if (s == "guided") request.options.feedbackStyle = InterviewFeedbackStyle::Guided;
        }

        if (options.contains("format") && options["format"].is_string()) {
            const auto s = options["format"].get<std::string>();
            if (s == "plain") request.options.format = InterviewMessageFormat::Plain;
            else if (s == "markdown") request.options.format = InterviewMessageFormat::Markdown;
        }

        if (options.contains("questionIndex") && options["questionIndex"].is_number_integer()) {
            request.options.questionIndex = options["questionIndex"].get<int>();
        }
        if (options.contains("questionCount") && options["questionCount"].is_number_integer()) {
            request.options.questionCount = options["questionCount"].get<int>();
        }
        if (options.contains("unknownAnswerStreak") && options["unknownAnswerStreak"].is_number_integer()) {
            request.options.unknownAnswerStreak = options["unknownAnswerStreak"].get<int>();
        }
        if (options.contains("forceRevealReferenceAnswer") && options["forceRevealReferenceAnswer"].is_boolean()) {
            request.options.forceRevealReferenceAnswer = options["forceRevealReferenceAnswer"].get<bool>();
        }
        if (options.contains("referenceAnswerHint") && options["referenceAnswerHint"].is_string()) {
            request.options.referenceAnswerHint = options["referenceAnswerHint"].get<std::string>();
        }
    }

    return request;
}

std::string InterviewStreamController::serializeSseEvent(const std::string& eventName, const std::string& data) {
    return "event: " + eventName + "\ndata: " + data + "\n\n";
}


void InterviewStreamController::streamInterview(const httplib::Request& req, httplib::Response& res) {
    try {
        const auto body = nlohmann::json::parse(req.body);
        const auto request = parseStreamRequest(body);

        if (request.sessionId.empty() ||
            request.messageId.empty() ||
            request.threadId.empty() ||
            request.topic.empty() ||
            request.topicLabel.empty() ||
            request.prompt.empty() ||
            request.questionTitle.empty() ||
            request.questionPrompt.empty() ||
            request.answer.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"missing required fields"})", "application/json");
            return;
        }

        res.set_header("Content-Type", "text/event-stream; charset=utf-8");
        res.set_header("Cache-Control", "no-cache, no-transform");
        res.set_header("Connection", "keep-alive");

        auto session = m_manager.createSession(request);
        const auto result = m_manager.submit(request, session);
        if (!result.accepted) {
            res.status = 429;
            res.set_content(nlohmann::json{{"success", false}, {"error", result.reason}}.dump(),
                            "application/json");
            return;
        }

        res.set_chunked_content_provider("text/event-stream",
            [this, session](size_t, httplib::DataSink& sink) {
                InterviewStreamEvent event;
                while (true) {
                    // A wait timeout means that the provider has not emitted a
                    // chunk yet; it must not terminate the SSE response.
                    if (!session->waitPop(event, 1000)) {
                        if (session->closed() || session->cancelled()) {
                            break;
                        }
                        continue;
                    }
                    if (event.type == InterviewStreamEventType::Chunk && event.content.has_value()) {
                        const auto payload = nlohmann::json{{"content", *event.content}}.dump();
                        const auto sse = serializeSseEvent("chunk", payload);
                        if (!sink.write(sse.c_str(), sse.size())) {
                            m_manager.cancel(session->sessionId(), session->threadId());
                            return false;
                        }
                    } else if (event.type == InterviewStreamEventType::Done) {
                        const auto sse = serializeSseEvent("done", "{}");
                        sink.write(sse.c_str(), sse.size());
                    } else if (event.type == InterviewStreamEventType::Error && event.error.has_value()) {
                        const auto payload = nlohmann::json{{"code", static_cast<int>(event.error->code)},
                                                            {"message", event.error->message}}.dump();
                        const auto sse = serializeSseEvent("error", payload);
                        sink.write(sse.c_str(), sse.size());
                    }
                }
                return false;
            });

    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(std::string("{\"success\":false,\"error\":\"") + e.what() + "\"}",
                        "application/json");
    }
}
