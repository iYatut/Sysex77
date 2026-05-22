/*
  ==============================================================================

    Sy99ParamApiServer.cpp

  ==============================================================================
*/

#include "Sy99ParamApiServer.h"

#include "Sy99ControllerTemplateStore.h"
#include "Sy99HardwareMappingStore.h"
#include "Sy99HardwareMappingRuntime.h"
#include "Sy99ParamRegistry.h"

namespace
{
    struct HttpRequest
    {
        juce::String method;
        juce::String path;
        juce::String body;
    };

    juce::String readRequest (juce::StreamingSocket& socket, HttpRequest& req)
    {
        juce::MemoryBlock buffer;
        char chunk[4096];

        for (int attempt = 0; attempt < 64; ++attempt)
        {
            const int ready = socket.waitUntilReady (true, 3000);

            if (ready == 0)
                break;

            if (ready < 0)
                return "socket read timeout";

            const int bytesRead = socket.read (chunk, (int) sizeof (chunk), false);

            if (bytesRead < 0)
                return "socket read failed";

            if (bytesRead == 0)
                break;

            buffer.append (chunk, (size_t) bytesRead);

            const juce::String text = buffer.toString();
            const int headerEnd = text.indexOf ("\r\n\r\n");

            if (headerEnd < 0)
                continue;

            const juce::String headersPart = text.substring (0, headerEnd);
            const juce::String bodyPart = text.substring (headerEnd + 4);
            const auto headerLines = juce::StringArray::fromLines (headersPart);

            if (headerLines.isEmpty())
                return "malformed request";

            const auto requestLine = juce::StringArray::fromTokens (headerLines[0], " ", "");
            if (requestLine.size() < 2)
                return "malformed request line";

            req.method = requestLine[0].toUpperCase();
            req.path = requestLine[1];

            int contentLength = 0;

            for (int i = 1; i < headerLines.size(); ++i)
            {
                const juce::String line = headerLines[i];

                if (line.startsWithIgnoreCase ("Content-Length:"))
                    contentLength = line.fromLastOccurrenceOf (":", false, false).trim().getIntValue();
            }

            req.body = bodyPart;

            while (req.body.getNumBytesAsUTF8() < contentLength)
            {
                const int moreReady = socket.waitUntilReady (true, 3000);

                if (moreReady <= 0)
                    return "incomplete request body";

                const int moreRead = socket.read (chunk, (int) sizeof (chunk), false);

                if (moreRead <= 0)
                    return "incomplete request body";

                req.body += juce::String::fromUTF8 (chunk, moreRead);
            }

            if (contentLength > 0)
                req.body = req.body.substring (0, contentLength);

            return {};
        }

        return "empty request";
    }

    void writeHttpResponse (juce::StreamingSocket& socket,
                            int statusCode,
                            const juce::String& statusText,
                            const juce::String& contentType,
                            const juce::String& body)
    {
        const juce::String response =
            "HTTP/1.1 " + juce::String (statusCode) + " " + statusText + "\r\n"
            "Content-Type: " + contentType + "\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, PUT, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: close\r\n"
            "Content-Length: " + juce::String (body.getNumBytesAsUTF8()) + "\r\n"
            "\r\n"
            + body;

        socket.write (response.toRawUTF8(), (size_t) response.getNumBytesAsUTF8());
    }

    void writeJson (juce::StreamingSocket& socket, int statusCode, const juce::var& jsonBody)
    {
        const juce::String body = juce::JSON::toString (jsonBody, true);
        juce::String statusText = "OK";

        if (statusCode == 404)
            statusText = "Not Found";
        else if (statusCode == 400)
            statusText = "Bad Request";
        else if (statusCode == 405)
            statusText = "Method Not Allowed";
        else if (statusCode >= 500)
            statusText = "Internal Server Error";

        writeHttpResponse (socket, statusCode, statusText, "application/json; charset=utf-8", body);
    }

    juce::String extractTemplateIdFromPath (const juce::String& path)
    {
        const juce::String prefix = "/api/controller-templates/";
        if (! path.startsWithIgnoreCase (prefix))
            return {};

        juce::String id = path.substring (prefix.length());

        if (const int query = id.indexOfChar ('?'); query >= 0)
            id = id.substring (0, query);

        return id.trim();
    }

    juce::String extractHardwareDeviceIdFromPath (const juce::String& path)
    {
        const juce::String prefix = "/api/hardware-mappings/";
        if (! path.startsWithIgnoreCase (prefix))
            return {};

        juce::String id = path.substring (prefix.length());

        if (const int query = id.indexOfChar ('?'); query >= 0)
            id = id.substring (0, query);

        return id.trim();
    }

    juce::String extractParamIdFromPath (const juce::String& path, bool stripValueMapSuffix = true)
    {
        const juce::String prefix = "/api/params/";
        if (! path.startsWithIgnoreCase (prefix))
            return {};

        juce::String id = path.substring (prefix.length());

        if (const int query = id.indexOfChar ('?'); query >= 0)
            id = id.substring (0, query);

        if (const int slash = id.indexOfChar ('/'); slash >= 0)
            id = id.substring (0, slash);

        return id.trim();
    }

    bool pathIsParamValueMap (const juce::String& path)
    {
        juce::String withoutQuery = path;

        if (const int query = withoutQuery.indexOfChar ('?'); query >= 0)
            withoutQuery = withoutQuery.substring (0, query);

        return withoutQuery.endsWithIgnoreCase ("/value-map");
    }

    bool pathIsParamCompareLog (const juce::String& path)
    {
        juce::String withoutQuery = path;

        if (const int query = withoutQuery.indexOfChar ('?'); query >= 0)
            withoutQuery = withoutQuery.substring (0, query);

        return withoutQuery.endsWithIgnoreCase ("/compare-log");
    }

    bool pathIsParamFocusSy99 (const juce::String& path)
    {
        juce::String withoutQuery = path;

        if (const int query = withoutQuery.indexOfChar ('?'); query >= 0)
            withoutQuery = withoutQuery.substring (0, query);

        return withoutQuery.endsWithIgnoreCase ("/focus-sy99");
    }

    int queryIntParam (const juce::String& path, const juce::String& key, int defaultValue)
    {
        const int queryStart = path.indexOfChar ('?');

        if (queryStart < 0)
            return defaultValue;

        const juce::String query = path.substring (queryStart + 1);
        const auto pairs = juce::StringArray::fromTokens (query, "&", "");

        for (const auto& pair : pairs)
        {
            const int eq = pair.indexOfChar ('=');

            if (eq < 0)
                continue;

            if (! pair.substring (0, eq).equalsIgnoreCase (key))
                continue;

            return pair.substring (eq + 1).trim().getIntValue();
        }

        return defaultValue;
    }

    uint8 querySysexDeviceByte (const juce::String& path)
    {
        const int device = queryIntParam (path, "sysexDevice", 16);
        return device == 16 ? (uint8) 0x10 : (uint8) juce::jlimit (0, 127, device);
    }

    juce::var errorJson (const juce::String& message,
                         const juce::StringPairArray& extra = {})
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("error", message);

        for (int i = 0; i < extra.size(); ++i)
            obj->setProperty (extra.getAllKeys()[i], extra.getAllValues()[i]);

        return obj;
    }

    void handleClient (juce::StreamingSocket& client)
    {
        HttpRequest req;
        const juce::String readError = readRequest (client, req);

        if (readError.isNotEmpty())
        {
            writeJson (client, 400, errorJson (readError));
            return;
        }

        if (req.method == "OPTIONS")
        {
            writeHttpResponse (client, 204, "No Content", "text/plain", {});
            return;
        }

        if (req.method == "GET" && req.path.equalsIgnoreCase ("/api/params"))
        {
            writeJson (client, 200, Sy99ParamRegistry::allParamsToJsonVar());
            return;
        }

        if (req.method == "GET" && req.path.equalsIgnoreCase ("/api/dump/current"))
        {
            writeJson (client, 200, Sy99ParamRegistry::currentLiveDumpToJsonVar());
            return;
        }

        if (req.method == "POST" && req.path.equalsIgnoreCase ("/api/focus-sy99"))
        {
            const juce::var body = req.body.isNotEmpty() ? juce::JSON::parse (req.body) : juce::var();
            const juce::String paramId = body.getProperty ("paramId", {}).toString();
            const int elementIndex = (int) body.getProperty ("elementIndex", 0);
            const int rawOverride = (int) body.getProperty ("raw", -1);
            const uint8 sysexDevice = (uint8) juce::jlimit (0, 127, (int) body.getProperty ("sysexDevice", 16));
            const Sy99ParamRegistry::Id id = Sy99ParamRegistry::idFromString (paramId);

            if (id >= Sy99ParamRegistry::Id::Count)
            {
                juce::StringPairArray extra;
                extra.set ("paramId", paramId);
                writeJson (client, 404, errorJson ("unknown parameter id", extra));
                return;
            }

            juce::String error;

            if (! Sy99ParamRegistry::focusSy99ParameterEditor (id, elementIndex, sysexDevice,
                                                               rawOverride, error))
            {
                writeJson (client, 409, errorJson (error.isNotEmpty() ? error
                                                                      : juce::String ("focus failed")));
                return;
            }

            auto* ok = new juce::DynamicObject();
            ok->setProperty ("ok", true);
            ok->setProperty ("paramId", paramId);
            ok->setProperty ("elementIndex", elementIndex);
            writeJson (client, 200, ok);
            return;
        }

        if (req.method == "GET" && req.path.equalsIgnoreCase ("/api/controller-templates"))
        {
            writeJson (client, 200, Sy99ControllerTemplates::allToJsonVar());
            return;
        }

        if (req.path.startsWithIgnoreCase ("/api/controller-templates/"))
        {
            const juce::String templateId = extractTemplateIdFromPath (req.path);

            if (templateId.isEmpty())
            {
                writeJson (client, 404, errorJson ("missing template id"));
                return;
            }

            if (req.method == "GET")
            {
                const juce::var tmpl = Sy99ControllerTemplates::getById (templateId);

                if (tmpl.isVoid())
                {
                    juce::StringPairArray extra;
                    extra.set ("id", templateId);
                    writeJson (client, 404, errorJson ("template not found", extra));
                    return;
                }

                writeJson (client, 200, tmpl);
                return;
            }

            const juce::var body = req.body.isNotEmpty()
                                     ? juce::JSON::parse (req.body)
                                     : juce::var();

            if (body.isVoid() || ! body.isObject())
            {
                writeJson (client, 400, errorJson ("expected JSON object body"));
                return;
            }

            if (req.method == "POST")
            {
                juce::String error;
                if (! Sy99ControllerTemplates::createTemplate (templateId, body, error))
                {
                    const int code = error.contains ("already exists") ? 409 : 400;
                    writeJson (client, code, errorJson (error));
                    return;
                }

                writeJson (client, 201, Sy99ControllerTemplates::getById (templateId));
                return;
            }

            if (req.method == "PUT")
            {
                juce::String error;
                if (! Sy99ControllerTemplates::upsertTemplate (templateId, body, error))
                {
                    writeJson (client, 400, errorJson (error));
                    return;
                }

                writeJson (client, 200, Sy99ControllerTemplates::getById (templateId));
                return;
            }

            writeJson (client, 405, errorJson ("method not allowed"));
            return;
        }

        if (req.method == "GET" && req.path.equalsIgnoreCase ("/api/hardware-mappings"))
        {
            writeJson (client, 200, Sy99HardwareMappings::allToJsonVar());
            return;
        }

        if (req.path.startsWithIgnoreCase ("/api/hardware-mappings/"))
        {
            const juce::String deviceId = extractHardwareDeviceIdFromPath (req.path);

            if (deviceId.isEmpty())
            {
                writeJson (client, 404, errorJson ("missing device id"));
                return;
            }

            if (req.method == "GET")
            {
                const juce::var mapping = Sy99HardwareMappings::getByDeviceId (deviceId);

                if (mapping.isVoid())
                {
                    juce::StringPairArray extra;
                    extra.set ("deviceId", deviceId);
                    writeJson (client, 404, errorJson ("mapping not found", extra));
                    return;
                }

                writeJson (client, 200, mapping);
                return;
            }

            const juce::var body = req.body.isNotEmpty()
                                     ? juce::JSON::parse (req.body)
                                     : juce::var();

            if (body.isVoid() || ! body.isObject())
            {
                writeJson (client, 400, errorJson ("expected JSON object body"));
                return;
            }

            if (req.method == "POST")
            {
                juce::String error;
                if (! Sy99HardwareMappings::createMapping (deviceId, body, error))
                {
                    const int code = error.contains ("already exists") ? 409 : 400;
                    writeJson (client, code, errorJson (error));
                    return;
                }

                Sy99HardwareMappingRuntime::reloadFromStore();
                writeJson (client, 201, Sy99HardwareMappings::getByDeviceId (deviceId));
                return;
            }

            if (req.method == "PUT")
            {
                juce::String error;
                if (! Sy99HardwareMappings::upsertMapping (deviceId, body, error))
                {
                    writeJson (client, 400, errorJson (error));
                    return;
                }

                Sy99HardwareMappingRuntime::reloadFromStore();
                writeJson (client, 200, Sy99HardwareMappings::getByDeviceId (deviceId));
                return;
            }

            writeJson (client, 405, errorJson ("method not allowed"));
            return;
        }

        if (req.path.startsWithIgnoreCase ("/api/params/"))
        {
            const bool wantsValueMap = pathIsParamValueMap (req.path);
            const bool wantsCompareLog = pathIsParamCompareLog (req.path);
            const bool wantsFocusSy99 = pathIsParamFocusSy99 (req.path);
            const juce::String paramId = extractParamIdFromPath (req.path,
                                                                  wantsValueMap || wantsCompareLog
                                                                      || wantsFocusSy99);
            const Sy99ParamRegistry::Id id = Sy99ParamRegistry::idFromString (paramId);

            if (id >= Sy99ParamRegistry::Id::Count)
            {
                juce::StringPairArray extra;
                extra.set ("id", paramId);
                writeJson (client, 404, errorJson ("unknown parameter id", extra));
                return;
            }

            if (req.method == "GET" && wantsValueMap)
            {
                const int elementIndex = queryIntParam (req.path, "elementIndex", 0);
                const uint8 sysexDevice = querySysexDeviceByte (req.path);
                const juce::var map = Sy99ParamRegistry::paramValueMapToJsonVar (id, elementIndex, sysexDevice);

                if (map.isVoid())
                {
                    writeJson (client, 404, errorJson ("parameter not found"));
                    return;
                }

                writeJson (client, 200, map);
                return;
            }

            if (req.method == "POST" && wantsCompareLog)
            {
                const int elementIndex = queryIntParam (req.path, "elementIndex", 0);
                const uint8 sysexDevice = querySysexDeviceByte (req.path);
                const juce::var body = req.body.isNotEmpty() ? juce::JSON::parse (req.body) : juce::var();
                const juce::String logText = body.getProperty ("logText", {}).toString();
                const juce::var overrides = body.getProperty ("rowOverrides", {});

                const juce::var result = Sy99ParamRegistry::compareObservationLogToJsonVar (
                    id, elementIndex, sysexDevice, logText, overrides);

                if (result.isVoid())
                {
                    writeJson (client, 404, errorJson ("parameter not found"));
                    return;
                }

                writeJson (client, 200, result);
                return;
            }

            if (req.method == "POST" && wantsFocusSy99)
            {
                const int elementIndex = queryIntParam (req.path, "elementIndex", 0);
                const uint8 sysexDevice = querySysexDeviceByte (req.path);
                const juce::var body = req.body.isNotEmpty() ? juce::JSON::parse (req.body) : juce::var();
                const int rawOverride = (int) body.getProperty ("raw", -1);
                juce::String error;

                if (! Sy99ParamRegistry::focusSy99ParameterEditor (id, elementIndex, sysexDevice,
                                                                   rawOverride, error))
                {
                    writeJson (client, 409, errorJson (error.isNotEmpty() ? error
                                                                      : juce::String ("focus failed")));
                    return;
                }

                auto* ok = new juce::DynamicObject();
                ok->setProperty ("ok", true);
                ok->setProperty ("paramId", paramId);
                ok->setProperty ("elementIndex", elementIndex);
                ok->setProperty ("message",
                                 "Sent current parameter value to SY99 (page jump, value unchanged)");
                writeJson (client, 200, ok);
                return;
            }

            if (req.method == "GET")
            {
                const juce::var record = Sy99ParamRegistry::metaRecordToJsonVar (id);

                if (record.isVoid())
                {
                    writeJson (client, 404, errorJson ("parameter not found"));
                    return;
                }

                writeJson (client, 200, record);
                return;
            }

            if (req.method == "PUT")
            {
                const juce::var patch = req.body.isNotEmpty()
                                          ? juce::JSON::parse (req.body)
                                          : juce::var();

                if (patch.isVoid() || ! patch.isObject())
                {
                    writeJson (client, 400, errorJson ("expected JSON object body"));
                    return;
                }

                if (! Sy99ParamRegistry::applyMetaPatch (id, patch))
                {
                    writeJson (client, 400, errorJson ("failed to apply metadata patch"));
                    return;
                }

                Sy99ParamRegistry::persistActiveMetaRegistry();

                const juce::var record = Sy99ParamRegistry::metaRecordToJsonVar (id);

                if (record.isVoid())
                {
                    writeJson (client, 500, errorJson ("patch applied but meta lookup failed"));
                    return;
                }

                writeJson (client, 200, record);
                return;
            }

            writeJson (client, 405, errorJson ("method not allowed"));
            return;
        }

        juce::StringPairArray extra;
        extra.set ("path", req.path);
        writeJson (client, 404, errorJson ("not found", extra));
    }
}

class Sy99ParamApiServer::ServerThread : public juce::Thread
{
public:
    explicit ServerThread (int port) : juce::Thread ("Sy99ParamApiServer"), listenPort (port) {}

    void run() override
    {
        juce::StreamingSocket listener;

        if (! listener.createListener (listenPort, "127.0.0.1"))
        {
            juce::Logger::writeToLog ("[ParamAPI] failed to listen on 127.0.0.1:"
                                      + juce::String (listenPort));
            return;
        }

        juce::Logger::writeToLog ("[ParamAPI] listening on http://127.0.0.1:"
                                  + juce::String (listenPort));

        while (! threadShouldExit())
        {
            std::unique_ptr<juce::StreamingSocket> client (listener.waitForNextConnection());

            if (client == nullptr)
                continue;

            handleClient (*client);
        }

        listener.close();
    }

private:
    int listenPort = Sy99ParamApiServer::kDefaultPort;
};

Sy99ParamApiServer::~Sy99ParamApiServer()
{
    stop();
}

void Sy99ParamApiServer::start (int port)
{
    stop();

    listenPort = port;
    serverThread = std::make_unique<ServerThread> (listenPort);
    running.store (true);
    serverThread->startThread();
}

void Sy99ParamApiServer::stop()
{
    if (serverThread != nullptr)
    {
        serverThread->signalThreadShouldExit();
        serverThread->stopThread (3000);
        serverThread.reset();
    }

    running.store (false);
}

Sy99ParamApiServer& getSy99ParamApiServer() noexcept
{
    static Sy99ParamApiServer instance;
    return instance;
}
