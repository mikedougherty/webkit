/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorFrontend.h"

#if ENABLE(INSPECTOR)

#include "ConsoleMessage.h"
#include "Frame.h"
#include "InjectedScript.h"
#include "InjectedScriptHost.h"
#include "InspectorClient.h"
#include "InspectorController.h"
#include "InspectorWorkerResource.h"
#include "Node.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include "ScriptString.h"
#include "ScriptValue.h"
#include "SerializedScriptValue.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

InspectorFrontend::InspectorFrontend(ScriptObject webInspector, InspectorClient* inspectorClient)
    : m_webInspector(webInspector)
    , m_inspectorClient(inspectorClient)
{
}

InspectorFrontend::~InspectorFrontend() 
{
    m_webInspector = ScriptObject();
}

void InspectorFrontend::close()
{
    ScriptFunctionCall function(m_webInspector, "close");
    function.call();
}

void InspectorFrontend::inspectedPageDestroyed()
{
    ScriptFunctionCall function(m_webInspector, "inspectedPageDestroyed");
    function.call();
}

ScriptArray InspectorFrontend::newScriptArray()
{
    return ScriptArray::createNew(scriptState());
}

ScriptObject InspectorFrontend::newScriptObject()
{
    return ScriptObject::createNew(scriptState());
}

void InspectorFrontend::didCommitLoad()
{
    callSimpleFunction("didCommitLoad");
}

void InspectorFrontend::populateApplicationSettings(const String& settings)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("populateApplicationSettings");
    function.appendArgument(settings);
    function.call();
}

void InspectorFrontend::populateSessionSettings(const String& settings)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("populateSessionSettings");
    function.appendArgument(settings);
    function.call();
}

void InspectorFrontend::updateConsoleMessageExpiredCount(unsigned count)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updateConsoleMessageExpiredCount");
    function.appendArgument(count);
    function.call();
}

void InspectorFrontend::addConsoleMessage(const ScriptObject& messageObj, const Vector<ScriptString>& frames, const Vector<RefPtr<SerializedScriptValue> >& arguments, const String& message)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addConsoleMessage");
    function.appendArgument(messageObj);
    if (!frames.isEmpty()) {
        for (unsigned i = 0; i < frames.size(); ++i)
            function.appendArgument(frames[i]);
    } else if (!arguments.isEmpty()) {
        for (unsigned i = 0; i < arguments.size(); ++i) {
            ScriptValue scriptValue = ScriptValue::deserialize(scriptState(), arguments[i].get());
            if (scriptValue.hasNoValue()) {
                ASSERT_NOT_REACHED();
                return;
            }
            function.appendArgument(scriptValue);
        }
    } else {
        function.appendArgument(message);
    }
    function.call();
}

void InspectorFrontend::updateConsoleMessageRepeatCount(unsigned count)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updateConsoleMessageRepeatCount");
    function.appendArgument(count);
    function.call();
}

void InspectorFrontend::clearConsoleMessages()
{
    callSimpleFunction("clearConsoleMessages");
}

bool InspectorFrontend::updateResource(unsigned long identifier, const ScriptObject& resourceObj)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updateResource");
    function.appendArgument(identifier);
    function.appendArgument(resourceObj);
    bool hadException = false;
    function.call(hadException);
    return !hadException;
}

void InspectorFrontend::removeResource(unsigned long identifier)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("removeResource");
    function.appendArgument(identifier);
    function.call();
}

void InspectorFrontend::didGetResourceContent(long callId, const String& content)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didGetResourceContent");
    function.appendArgument(callId);
    function.appendArgument(content);
    function.call();
}

void InspectorFrontend::updateFocusedNode(long nodeId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updateFocusedNode");
    function.appendArgument(nodeId);
    function.call();
}

void InspectorFrontend::setAttachedWindow(bool attached)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("setAttachedWindow");
    function.appendArgument(attached);
    function.call();
}

void InspectorFrontend::showPanel(int panel)
{
    const char* showFunctionName;
    switch (panel) {
        case InspectorController::AuditsPanel:
            showFunctionName = "showAuditsPanel";
            break;
        case InspectorController::ConsolePanel:
            showFunctionName = "showConsolePanel";
            break;
        case InspectorController::ElementsPanel:
            showFunctionName = "showElementsPanel";
            break;
        case InspectorController::ResourcesPanel:
            showFunctionName = "showResourcesPanel";
            break;
        case InspectorController::TimelinePanel:
            showFunctionName = "showTimelinePanel";
            break;
        case InspectorController::ProfilesPanel:
            showFunctionName = "showProfilesPanel";
            break;
        case InspectorController::ScriptsPanel:
            showFunctionName = "showScriptsPanel";
            break;
        case InspectorController::StoragePanel:
            showFunctionName = "showStoragePanel";
            break;
        default:
            ASSERT_NOT_REACHED();
            showFunctionName = 0;
    }

    if (showFunctionName)
        callSimpleFunction(showFunctionName);
}

void InspectorFrontend::populateInterface()
{
    callSimpleFunction("populateInterface");
}

void InspectorFrontend::reset()
{
    callSimpleFunction("reset");
}

void InspectorFrontend::bringToFront()
{
    callSimpleFunction("bringToFront");
}

void InspectorFrontend::inspectedURLChanged(const String& url)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("inspectedURLChanged");
    function.appendArgument(url);
    function.call();
}

void InspectorFrontend::resourceTrackingWasEnabled()
{
    callSimpleFunction("resourceTrackingWasEnabled");
}

void InspectorFrontend::resourceTrackingWasDisabled()
{
    callSimpleFunction("resourceTrackingWasDisabled");
}


void InspectorFrontend::searchingForNodeWasEnabled()
{
    callSimpleFunction("searchingForNodeWasEnabled");
}

void InspectorFrontend::searchingForNodeWasDisabled()
{
    callSimpleFunction("searchingForNodeWasDisabled");
}

void InspectorFrontend::updatePauseOnExceptionsState(long state)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updatePauseOnExceptionsState");
    function.appendArgument(state);
    function.call();
}

void InspectorFrontend::timelineProfilerWasStarted()
{
    callSimpleFunction("timelineProfilerWasStarted");
}

void InspectorFrontend::timelineProfilerWasStopped()
{
    callSimpleFunction("timelineProfilerWasStopped");
}

void InspectorFrontend::addRecordToTimeline(const ScriptObject& record)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addRecordToTimeline");
    function.appendArgument(record);
    function.call();
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void InspectorFrontend::attachDebuggerWhenShown()
{
    callSimpleFunction("attachDebuggerWhenShown");
}

void InspectorFrontend::debuggerWasEnabled()
{
    callSimpleFunction("debuggerWasEnabled");
}

void InspectorFrontend::debuggerWasDisabled()
{
    callSimpleFunction("debuggerWasDisabled");
}

void InspectorFrontend::parsedScriptSource(const String& sourceID, const String& url, const String& data, int firstLine, int scriptWorldType)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("parsedScriptSource");
    function.appendArgument(sourceID);
    function.appendArgument(url);
    function.appendArgument(data);
    function.appendArgument(firstLine);
    function.appendArgument(scriptWorldType);
    function.call();
}

void InspectorFrontend::restoredBreakpoint(const String& sourceID, const String& url, int line, bool enabled, const String& condition)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("restoredBreakpoint");
    function.appendArgument(sourceID);
    function.appendArgument(url);
    function.appendArgument(line);
    function.appendArgument(enabled);
    function.appendArgument(condition);
    function.call();
}

void InspectorFrontend::failedToParseScriptSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("failedToParseScriptSource");
    function.appendArgument(url);
    function.appendArgument(data);
    function.appendArgument(firstLine);
    function.appendArgument(errorLine);
    function.appendArgument(errorMessage);
    function.call();
}

void InspectorFrontend::pausedScript(SerializedScriptValue* callFrames)
{
    ScriptValue callFramesValue = ScriptValue::deserialize(scriptState(), callFrames);
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("pausedScript");
    function.appendArgument(callFramesValue);
    function.call();
}

void InspectorFrontend::resumedScript()
{
    callSimpleFunction("resumedScript");
}

void InspectorFrontend::didEditScriptSource(long callId, bool success, const String& result, SerializedScriptValue* newCallFrames)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didEditScriptSource");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.appendArgument(result);
    if (success && newCallFrames) {
        ScriptValue newCallFramesValue = ScriptValue::deserialize(scriptState(), newCallFrames);
        ASSERT(!newCallFramesValue .hasNoValue());
        function.appendArgument(newCallFramesValue);
    }
    function.call();
}

void InspectorFrontend::didGetScriptSource(long callId, const String& result)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didGetScriptSource");
    function.appendArgument(callId);
    function.appendArgument(result);
    function.call();
}

void InspectorFrontend::profilerWasEnabled()
{
    callSimpleFunction("profilerWasEnabled");
}

void InspectorFrontend::profilerWasDisabled()
{
    callSimpleFunction("profilerWasDisabled");
}

void InspectorFrontend::addProfileHeader(const ScriptValue& profile)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addProfileHeader");
    function.appendArgument(profile);
    function.call();
}

void InspectorFrontend::setRecordingProfile(bool isProfiling)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("setRecordingProfile");
    function.appendArgument(isProfiling);
    function.call();
}

void InspectorFrontend::didGetProfileHeaders(long callId, const ScriptArray& headers)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetProfileHeaders");
    function.appendArgument(callId);
    function.appendArgument(headers);
    function.call();
}

void InspectorFrontend::didGetProfile(long callId, const ScriptValue& profile)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetProfile");
    function.appendArgument(callId);
    function.appendArgument(profile);
    function.call();
}
#endif

void InspectorFrontend::setDocument(const ScriptObject& root)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("setDocument");
    function.appendArgument(root);
    function.call();
}

void InspectorFrontend::setDetachedRoot(const ScriptObject& root)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("setDetachedRoot");
    function.appendArgument(root);
    function.call();
}

void InspectorFrontend::setChildNodes(long parentId, const ScriptArray& nodes)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("setChildNodes");
    function.appendArgument(parentId);
    function.appendArgument(nodes);
    function.call();
}

void InspectorFrontend::childNodeCountUpdated(long id, int newValue)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("childNodeCountUpdated");
    function.appendArgument(id);
    function.appendArgument(newValue);
    function.call();
}

void InspectorFrontend::childNodeInserted(long parentId, long prevId, const ScriptObject& node)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("childNodeInserted");
    function.appendArgument(parentId);
    function.appendArgument(prevId);
    function.appendArgument(node);
    function.call();
}

void InspectorFrontend::childNodeRemoved(long parentId, long id)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("childNodeRemoved");
    function.appendArgument(parentId);
    function.appendArgument(id);
    function.call();
}

void InspectorFrontend::attributesUpdated(long id, const ScriptArray& attributes)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("attributesUpdated");
    function.appendArgument(id);
    function.appendArgument(attributes);
    function.call();
}

void InspectorFrontend::didRemoveNode(long callId, long nodeId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didRemoveNode");
    function.appendArgument(callId);
    function.appendArgument(nodeId);
    function.call();
}

void InspectorFrontend::didChangeTagName(long callId, long nodeId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didChangeTagName");
    function.appendArgument(callId);
    function.appendArgument(nodeId);
    function.call();
}

void InspectorFrontend::didGetOuterHTML(long callId, const String& outerHTML)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetOuterHTML");
    function.appendArgument(callId);
    function.appendArgument(outerHTML);
    function.call();
}

void InspectorFrontend::didSetOuterHTML(long callId, long nodeId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didSetOuterHTML");
    function.appendArgument(callId);
    function.appendArgument(nodeId);
    function.call();
}

void InspectorFrontend::didGetChildNodes(long callId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetChildNodes");
    function.appendArgument(callId);
    function.call();
}

void InspectorFrontend::didApplyDomChange(long callId, bool success)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didApplyDomChange");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.call();
}

void InspectorFrontend::didGetEventListenersForNode(long callId, long nodeId, const ScriptArray& listenersArray)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetEventListenersForNode");
    function.appendArgument(callId);
    function.appendArgument(nodeId);
    function.appendArgument(listenersArray);
    function.call();
}

void InspectorFrontend::didGetStyles(long callId, const ScriptValue& styles)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetStyles");
    function.appendArgument(callId);
    function.appendArgument(styles);
    function.call();
}

void InspectorFrontend::didGetAllStyles(long callId, const ScriptArray& styles)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didGetAllStyles");
    function.appendArgument(callId);
    function.appendArgument(styles);
    function.call();
}

void InspectorFrontend::didGetStyleSheet(long callId, const ScriptValue& styleSheet)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didGetStyleSheet");
    function.appendArgument(callId);
    function.appendArgument(styleSheet);
    function.call();
}

void InspectorFrontend::didGetComputedStyle(long callId, const ScriptValue& style)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetComputedStyle");
    function.appendArgument(callId);
    function.appendArgument(style);
    function.call();
}

void InspectorFrontend::didGetInlineStyle(long callId, const ScriptValue& style)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetInlineStyle");
    function.appendArgument(callId);
    function.appendArgument(style);
    function.call();
}

void InspectorFrontend::didApplyStyleText(long callId, bool success, const ScriptValue& style, const ScriptArray& changedProperties)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didApplyStyleText");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.appendArgument(style);
    function.appendArgument(changedProperties);
    function.call();
}

void InspectorFrontend::didSetStyleText(long callId, bool success)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didSetStyleText");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.call();
}

void InspectorFrontend::didSetStyleProperty(long callId, bool success)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didSetStyleProperty");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.call();
}

void InspectorFrontend::didToggleStyleEnabled(long callId, const ScriptValue& style)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didToggleStyleEnabled");
    function.appendArgument(callId);
    function.appendArgument(style);
    function.call();
}

void InspectorFrontend::didSetRuleSelector(long callId, const ScriptValue& rule, bool selectorAffectsNode)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didSetRuleSelector");
    function.appendArgument(callId);
    function.appendArgument(rule);
    function.appendArgument(selectorAffectsNode);
    function.call();
}

void InspectorFrontend::didAddRule(long callId, const ScriptValue& rule, bool selectorAffectsNode)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didAddRule");
    function.appendArgument(callId);
    function.appendArgument(rule);
    function.appendArgument(selectorAffectsNode);
    function.call();
}

#if ENABLE(WORKERS)
void InspectorFrontend::didCreateWorker(const InspectorWorkerResource& worker)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("didCreateWorker");
    function.appendArgument(worker.id());
    function.appendArgument(worker.url());
    function.appendArgument(worker.isSharedWorker());
    function.call();
}

void InspectorFrontend::didDestroyWorker(const InspectorWorkerResource& worker)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didDestroyWorker");
    function.appendArgument(worker.id());
    function.call();
}
#endif // ENABLE(WORKERS)

void InspectorFrontend::didGetCookies(long callId, const ScriptArray& cookies, const String& cookiesString)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetCookies");
    function.appendArgument(callId);
    function.appendArgument(cookies);
    function.appendArgument(cookiesString);
    function.call();
}

void InspectorFrontend::didDispatchOnInjectedScript(long callId, SerializedScriptValue* result, bool isException)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didDispatchOnInjectedScript");
    function.appendArgument(callId);
    if (isException)
        function.appendArgument("");
    else {
        ScriptValue resultValue = ScriptValue::deserialize(scriptState(), result);
        function.appendArgument(resultValue);
    }
    function.appendArgument(isException);
    function.call();
}

#if ENABLE(DATABASE)
bool InspectorFrontend::addDatabase(const ScriptObject& dbObject)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addDatabase");
    function.appendArgument(dbObject);
    bool hadException = false;
    function.call(hadException);
    return !hadException;
}

void InspectorFrontend::selectDatabase(int databaseId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("selectDatabase");
    function.appendArgument(databaseId);
    function.call();
}

void InspectorFrontend::didGetDatabaseTableNames(long callId, const ScriptArray& tableNames)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetDatabaseTableNames");
    function.appendArgument(callId);
    function.appendArgument(tableNames);
    function.call();
}
#endif

#if ENABLE(DOM_STORAGE)
bool InspectorFrontend::addDOMStorage(const ScriptObject& domStorageObj)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addDOMStorage");
    function.appendArgument(domStorageObj);
    bool hadException = false;
    function.call(hadException);
    return !hadException;
}

void InspectorFrontend::selectDOMStorage(long storageId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("selectDOMStorage");
    function.appendArgument(storageId);
    function.call();
}

void InspectorFrontend::didGetDOMStorageEntries(long callId, const ScriptArray& entries)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didGetDOMStorageEntries");
    function.appendArgument(callId);
    function.appendArgument(entries);
    function.call();
}

void InspectorFrontend::didSetDOMStorageItem(long callId, bool success)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didSetDOMStorageItem");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.call();
}

void InspectorFrontend::didRemoveDOMStorageItem(long callId, bool success)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("didRemoveDOMStorageItem");
    function.appendArgument(callId);
    function.appendArgument(success);
    function.call();
}

void InspectorFrontend::updateDOMStorage(long storageId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("updateDOMStorage");
    function.appendArgument(storageId);
    function.call();
}
#endif

void InspectorFrontend::addNodesToSearchResult(const ScriptArray& nodeIds)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("addNodesToSearchResult");
    function.appendArgument(nodeIds);
    function.call();
}

void InspectorFrontend::contextMenuItemSelected(int itemId)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument("contextMenuItemSelected");
    function.appendArgument(itemId);
    function.call();
}

void InspectorFrontend::contextMenuCleared()
{
    callSimpleFunction("contextMenuCleared");
}

void InspectorFrontend::evaluateForTestInFrontend(long callId, const String& script)
{
    ScriptFunctionCall function(m_webInspector, "dispatch"); 
    function.appendArgument("evaluateForTestInFrontend");
    function.appendArgument(callId);
    function.appendArgument(script);
    function.call();
}

void InspectorFrontend::callSimpleFunction(const String& functionName)
{
    ScriptFunctionCall function(m_webInspector, "dispatch");
    function.appendArgument(functionName);
    function.call();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
