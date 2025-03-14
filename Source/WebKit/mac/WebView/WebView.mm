/*
 * Copyright (C) 2005-2013 Apple Inc. All rights reserved.
 * Copyright (C) 2006 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Igalia S.L
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#import "WebViewInternal.h"
#import "WebViewData.h"

#import "DOMCSSStyleDeclarationInternal.h"
#import "DOMDocumentInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "DictionaryPopupInfo.h"
#import "StorageThread.h"
#import "WebAlternativeTextClient.h"
#import "WebApplicationCache.h"
#import "WebBackForwardListInternal.h"
#import "WebBaseNetscapePluginView.h"
#import "WebCache.h"
#import "WebChromeClient.h"
#import "WebDOMOperationsPrivate.h"
#import "WebDataSourceInternal.h"
#import "WebDatabaseManagerPrivate.h"
#import "WebDatabaseProvider.h"
#import "WebDefaultEditingDelegate.h"
#import "WebDefaultPolicyDelegate.h"
#import "WebDefaultUIDelegate.h"
#import "WebDelegateImplementationCaching.h"
#import "WebDeviceOrientationClient.h"
#import "WebDeviceOrientationProvider.h"
#import "WebDocument.h"
#import "WebDocumentInternal.h"
#import "WebDownload.h"
#import "WebDownloadInternal.h"
#import "WebDragClient.h"
#import "WebDynamicScrollBarsViewInternal.h"
#import "WebEditingDelegate.h"
#import "WebEditorClient.h"
#import "WebFormDelegatePrivate.h"
#import "WebFrameInternal.h"
#import "WebFrameLoaderClient.h"
#import "WebFrameNetworkingContext.h"
#import "WebFrameViewInternal.h"
#import "WebGeolocationClient.h"
#import "WebGeolocationPositionInternal.h"
#import "WebHTMLRepresentation.h"
#import "WebHTMLViewInternal.h"
#import "WebHistoryItemInternal.h"
#import "WebIconDatabaseInternal.h"
#import "WebInspector.h"
#import "WebInspectorClient.h"
#import "WebKitErrors.h"
#import "WebKitFullScreenListener.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebKitStatisticsPrivate.h"
#import "WebKitSystemBits.h"
#import "WebKitVersionChecks.h"
#import "WebLocalizableStrings.h"
#import "WebNSDataExtras.h"
#import "WebNSDataExtrasPrivate.h"
#import "WebNSDictionaryExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebNSViewExtras.h"
#import "WebNodeHighlight.h"
#import "WebNotificationClient.h"
#import "WebPDFView.h"
#import "WebPanelAuthenticationHandler.h"
#import "WebPlatformStrategies.h"
#import "WebPluginDatabase.h"
#import "WebPolicyDelegate.h"
#import "WebPreferenceKeysPrivate.h"
#import "WebPreferencesPrivate.h"
#import "WebProgressTrackerClient.h"
#import "WebScriptDebugDelegate.h"
#import "WebScriptWorldInternal.h"
#import "WebSelectionServiceController.h"
#import "WebStorageManagerInternal.h"
#import "WebStorageNamespaceProvider.h"
#import "WebSystemInterface.h"
#import "WebTextCompletionController.h"
#import "WebTextIterator.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebUserMediaClient.h"
#import "WebViewGroup.h"
#import "WebVisitedLinkStore.h"
#import <CoreFoundation/CFSet.h>
#import <Foundation/NSURLConnection.h>
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSValueRef.h>
#import <WebCore/AlternativeTextUIController.h>
#import <WebCore/AnimationController.h>
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/BackForwardController.h>
#import <WebCore/BackForwardList.h>
#import <WebCore/CFNetworkSPI.h>
#import <WebCore/Chrome.h>
#import <WebCore/ColorMac.h>
#import <WebCore/Cursor.h>
#import <WebCore/DatabaseManager.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/DragController.h>
#import <WebCore/DragData.h>
#import <WebCore/Editor.h>
#import <WebCore/EventHandler.h>
#import <WebCore/ExceptionHandlers.h>
#import <WebCore/FocusController.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameSelection.h>
#import <WebCore/FrameTree.h>
#import <WebCore/FrameView.h>
#import <WebCore/GCController.h>
#import <WebCore/GeolocationController.h>
#import <WebCore/GeolocationError.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLVideoElement.h>
#import <WebCore/HistoryController.h>
#import <WebCore/HistoryItem.h>
#import <WebCore/IconDatabase.h>
#import <WebCore/JSCSSStyleDeclaration.h>
#import <WebCore/JSDocument.h>
#import <WebCore/JSElement.h>
#import <WebCore/JSNodeList.h>
#import <WebCore/JSNotification.h>
#import <WebCore/Logging.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/MainFrame.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/MemoryPressureHandler.h>
#import <WebCore/NSURLFileTypeMappingsSPI.h>
#import <WebCore/NodeList.h>
#import <WebCore/Notification.h>
#import <WebCore/NotificationController.h>
#import <WebCore/Page.h>
#import <WebCore/PageCache.h>
#import <WebCore/PageConfiguration.h>
#import <WebCore/PageGroup.h>
#import <WebCore/PlatformEventFactoryMac.h>
#import <WebCore/ProgressTracker.h>
#import <WebCore/RenderView.h>
#import <WebCore/RenderWidget.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ResourceLoadScheduler.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/RuntimeEnabledFeatures.h>
#import <WebCore/SchemeRegistry.h>
#import <WebCore/ScriptController.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SecurityPolicy.h>
#import <WebCore/Settings.h>
#import <WebCore/StyleProperties.h>
#import <WebCore/TextResourceDecoder.h>
#import <WebCore/ThreadCheck.h>
#import <WebCore/UserAgent.h>
#import <WebCore/UserContentController.h>
#import <WebCore/UserScript.h>
#import <WebCore/UserStyleSheet.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreView.h>
#import <WebCore/Widget.h>
#import <WebKitLegacy/DOM.h>
#import <WebKitLegacy/DOMExtensions.h>
#import <WebKitLegacy/DOMPrivate.h>
#import <WebKitSystemInterface.h>
#import <bindings/ScriptValue.h>
#import <mach-o/dyld.h>
#import <objc/objc-auto.h>
#import <objc/runtime.h>
#import <runtime/ArrayPrototype.h>
#import <runtime/DateInstance.h>
#import <runtime/InitializeThreading.h>
#import <runtime/JSCJSValue.h>
#import <runtime/JSLock.h>
#import <wtf/Assertions.h>
#import <wtf/HashTraits.h>
#import <wtf/MainThread.h>
#import <wtf/ObjcRuntimeExtras.h>
#import <wtf/RefCountedLeakCounter.h>
#import <wtf/RefPtr.h>
#import <wtf/RunLoop.h>
#import <wtf/StdLibExtras.h>

#if !PLATFORM(IOS)
#import "WebActionMenuController.h"
#import "WebContextMenuClient.h"
#import "WebFullScreenController.h"
#import "WebImmediateActionController.h"
#import "WebNSEventExtras.h"
#import "WebNSObjectExtras.h"
#import "WebNSPasteboardExtras.h"
#import "WebNSPrintOperationExtras.h"
#import "WebPDFView.h"
#import <WebCore/LookupSPI.h>
#import <WebCore/NSImmediateActionGestureRecognizerSPI.h>
#import <WebCore/NSViewSPI.h>
#import <WebCore/SoftLinking.h>
#import <WebCore/TextIndicator.h>
#import <WebCore/TextIndicatorWindow.h>
#import <WebCore/WebVideoFullscreenController.h>
#else
#import "MemoryMeasure.h"
#import "WebCaretChangeListener.h"
#import "WebChromeClientIOS.h"
#import "WebDefaultFormDelegate.h"
#import "WebDefaultFrameLoadDelegate.h"
#import "WebDefaultResourceLoadDelegate.h"
#import "WebDefaultUIKitDelegate.h"
#import "WebFixedPositionContent.h"
#import "WebMailDelegate.h"
#import "WebNSUserDefaultsExtras.h"
#import "WebPDFViewIOS.h"
#import "WebPlainWhiteView.h"
#import "WebPluginController.h"
#import "WebPolicyDelegatePrivate.h"
#import "WebSQLiteDatabaseTrackerClient.h"
#import "WebStorageManagerPrivate.h"
#import "WebUIKitSupport.h"
#import "WebVisiblePosition.h"
#import <WebCore/DispatchSPI.h>
#import <WebCore/EventNames.h>
#import <WebCore/FontCache.h>
#import <WebCore/GraphicsLayer.h>
#import <WebCore/IconController.h>
#import <WebCore/LegacyTileCache.h>
#import <WebCore/MobileGestaltSPI.h>
#import <WebCore/NetworkStateNotifier.h>
#import <WebCore/RuntimeApplicationChecksIOS.h>
#import <WebCore/SQLiteDatabaseTracker.h>
#import <WebCore/SmartReplace.h>
#import <WebCore/TextRun.h>
#import <WebCore/TileControllerMemoryHandlerIOS.h>
#import <WebCore/WAKWindow.h>
#import <WebCore/WKView.h>
#import <WebCore/WebCoreThread.h>
#import <WebCore/WebCoreThreadMessage.h>
#import <WebCore/WebCoreThreadRun.h>
#import <WebCore/WebEvent.h>
#import <WebCore/WebVideoFullscreenControllerAVKit.h>
#import <wtf/FastMalloc.h>
#endif // !PLATFORM(IOS)

#if ENABLE(DASHBOARD_SUPPORT)
#import <WebKitLegacy/WebDashboardRegion.h>
#endif

#if ENABLE(REMOTE_INSPECTOR)
#import <JavaScriptCore/RemoteInspector.h>
#if PLATFORM(IOS)
#import "WebIndicateLayer.h"
#endif
#endif

#if USE(QUICK_LOOK)
#include <WebCore/QuickLook.h>
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
#import <WebCore/WebEventRegion.h>
#endif

#if ENABLE(GAMEPAD)
#import <WebCore/HIDGamepadProvider.h>
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
#import "WebMediaPlaybackTargetPicker.h"
#endif

#if PLATFORM(MAC)
SOFT_LINK_CONSTANT_MAY_FAIL(Lookup, LUNotificationPopoverWillClose, NSString *)
SOFT_LINK_CONSTANT_MAY_FAIL(Lookup, LUTermOptionDisableSearchTermIndicator, NSString *)
#endif

#if !PLATFORM(IOS)
@interface NSSpellChecker (WebNSSpellCheckerDetails)
- (void)_preflightChosenSpellServer;
@end

@interface NSView (WebNSViewDetails)
- (NSView *)_hitTest:(NSPoint *)aPoint dragTypes:(NSSet *)types;
- (void)_autoscrollForDraggingInfo:(id)dragInfo timeDelta:(NSTimeInterval)repeatDelta;
- (BOOL)_shouldAutoscrollForDraggingInfo:(id)dragInfo;
- (void)_windowChangedKeyState;
@end

@interface NSWindow (WebNSWindowDetails)
- (id)_oldFirstResponderBeforeBecoming;
- (void)_enableScreenUpdatesIfNeeded;
- (BOOL)_wrapsCarbonWindow;
- (BOOL)_hasKeyAppearance;
@end
#endif

using namespace JSC;
using namespace Inspector;
using namespace WebCore;

#define FOR_EACH_RESPONDER_SELECTOR(macro) \
macro(alignCenter) \
macro(alignJustified) \
macro(alignLeft) \
macro(alignRight) \
macro(capitalizeWord) \
macro(centerSelectionInVisibleArea) \
macro(changeAttributes) \
macro(changeBaseWritingDirection) \
macro(changeBaseWritingDirectionToLTR) \
macro(changeBaseWritingDirectionToRTL) \
macro(changeColor) \
macro(changeDocumentBackgroundColor) \
macro(changeFont) \
macro(changeSpelling) \
macro(checkSpelling) \
macro(complete) \
macro(copy) \
macro(copyFont) \
macro(cut) \
macro(delete) \
macro(deleteBackward) \
macro(deleteBackwardByDecomposingPreviousCharacter) \
macro(deleteForward) \
macro(deleteToBeginningOfLine) \
macro(deleteToBeginningOfParagraph) \
macro(deleteToEndOfLine) \
macro(deleteToEndOfParagraph) \
macro(deleteToMark) \
macro(deleteWordBackward) \
macro(deleteWordForward) \
macro(ignoreSpelling) \
macro(indent) \
macro(insertBacktab) \
macro(insertLineBreak) \
macro(insertNewline) \
macro(insertNewlineIgnoringFieldEditor) \
macro(insertParagraphSeparator) \
macro(insertTab) \
macro(insertTabIgnoringFieldEditor) \
macro(lowercaseWord) \
macro(makeBaseWritingDirectionLeftToRight) \
macro(makeBaseWritingDirectionRightToLeft) \
macro(makeTextWritingDirectionLeftToRight) \
macro(makeTextWritingDirectionNatural) \
macro(makeTextWritingDirectionRightToLeft) \
macro(moveBackward) \
macro(moveBackwardAndModifySelection) \
macro(moveDown) \
macro(moveDownAndModifySelection) \
macro(moveForward) \
macro(moveForwardAndModifySelection) \
macro(moveLeft) \
macro(moveLeftAndModifySelection) \
macro(moveParagraphBackwardAndModifySelection) \
macro(moveParagraphForwardAndModifySelection) \
macro(moveRight) \
macro(moveRightAndModifySelection) \
macro(moveToBeginningOfDocument) \
macro(moveToBeginningOfDocumentAndModifySelection) \
macro(moveToBeginningOfLine) \
macro(moveToBeginningOfLineAndModifySelection) \
macro(moveToBeginningOfParagraph) \
macro(moveToBeginningOfParagraphAndModifySelection) \
macro(moveToBeginningOfSentence) \
macro(moveToBeginningOfSentenceAndModifySelection) \
macro(moveToEndOfDocument) \
macro(moveToEndOfDocumentAndModifySelection) \
macro(moveToEndOfLine) \
macro(moveToEndOfLineAndModifySelection) \
macro(moveToEndOfParagraph) \
macro(moveToEndOfParagraphAndModifySelection) \
macro(moveToEndOfSentence) \
macro(moveToEndOfSentenceAndModifySelection) \
macro(moveToLeftEndOfLine) \
macro(moveToLeftEndOfLineAndModifySelection) \
macro(moveToRightEndOfLine) \
macro(moveToRightEndOfLineAndModifySelection) \
macro(moveUp) \
macro(moveUpAndModifySelection) \
macro(moveWordBackward) \
macro(moveWordBackwardAndModifySelection) \
macro(moveWordForward) \
macro(moveWordForwardAndModifySelection) \
macro(moveWordLeft) \
macro(moveWordLeftAndModifySelection) \
macro(moveWordRight) \
macro(moveWordRightAndModifySelection) \
macro(orderFrontSubstitutionsPanel) \
macro(outdent) \
macro(overWrite) \
macro(pageDown) \
macro(pageDownAndModifySelection) \
macro(pageUp) \
macro(pageUpAndModifySelection) \
macro(paste) \
macro(pasteAsPlainText) \
macro(pasteAsRichText) \
macro(pasteFont) \
macro(performFindPanelAction) \
macro(scrollLineDown) \
macro(scrollLineUp) \
macro(scrollPageDown) \
macro(scrollPageUp) \
macro(scrollToBeginningOfDocument) \
macro(scrollToEndOfDocument) \
macro(selectAll) \
macro(selectLine) \
macro(selectParagraph) \
macro(selectSentence) \
macro(selectToMark) \
macro(selectWord) \
macro(setMark) \
macro(showGuessPanel) \
macro(startSpeaking) \
macro(stopSpeaking) \
macro(subscript) \
macro(superscript) \
macro(swapWithMark) \
macro(takeFindStringFromSelection) \
macro(toggleBaseWritingDirection) \
macro(transpose) \
macro(underline) \
macro(unscript) \
macro(uppercaseWord) \
macro(yank) \
macro(yankAndSelect) \

#define WebKitOriginalTopPrintingMarginKey @"WebKitOriginalTopMargin"
#define WebKitOriginalBottomPrintingMarginKey @"WebKitOriginalBottomMargin"

#define KeyboardUIModeDidChangeNotification @"com.apple.KeyboardUIModeDidChange"
#define AppleKeyboardUIMode CFSTR("AppleKeyboardUIMode")

static BOOL s_didSetCacheModel;
static WebCacheModel s_cacheModel = WebCacheModelDocumentViewer;

#if PLATFORM(IOS)
static Class s_pdfRepresentationClass;
static Class s_pdfViewClass;
#endif

#ifndef NDEBUG
static const char webViewIsOpen[] = "At least one WebView is still open.";
#endif

#if PLATFORM(IOS)
@interface WebView(WebViewPrivate)
- (void)_preferencesChanged:(WebPreferences *)preferences;
- (void)_updateScreenScaleFromWindow;
@end

@interface NSURLCache (WebPrivate)
- (CFURLCacheRef)_CFURLCache;
@end
#endif

#if !PLATFORM(IOS)
@interface NSObject (WebValidateWithoutDelegate)
- (BOOL)validateUserInterfaceItemWithoutDelegate:(id <NSValidatedUserInterfaceItem>)item;
@end
#endif

#if PLATFORM(IOS)
@class _WebSafeForwarder;

@interface _WebSafeAsyncForwarder : NSObject {
    _WebSafeForwarder *_forwarder;
}
- (id)initWithForwarder:(_WebSafeForwarder *)forwarder;
@end
#endif

@interface _WebSafeForwarder : NSObject
{
    id target; // Non-retained. Don't retain delegates.
    id defaultTarget;
#if PLATFORM(IOS)
    _WebSafeAsyncForwarder *asyncForwarder;
    dispatch_once_t asyncForwarderPred;
#endif
}
- (instancetype)initWithTarget:(id)target defaultTarget:(id)defaultTarget;
#if PLATFORM(IOS)
- (void)clearTarget;
- (id)asyncForwarder;
#endif
@end

FindOptions coreOptions(WebFindOptions options)
{
    return (options & WebFindOptionsCaseInsensitive ? CaseInsensitive : 0)
        | (options & WebFindOptionsAtWordStarts ? AtWordStarts : 0)
        | (options & WebFindOptionsTreatMedialCapitalAsWordStart ? TreatMedialCapitalAsWordStart : 0)
        | (options & WebFindOptionsBackwards ? Backwards : 0)
        | (options & WebFindOptionsWrapAround ? WrapAround : 0)
        | (options & WebFindOptionsStartInSelection ? StartInSelection : 0);
}

LayoutMilestones coreLayoutMilestones(WebLayoutMilestones milestones)
{
    return (milestones & WebDidFirstLayout ? DidFirstLayout : 0)
        | (milestones & WebDidFirstVisuallyNonEmptyLayout ? DidFirstVisuallyNonEmptyLayout : 0)
        | (milestones & WebDidHitRelevantRepaintedObjectsAreaThreshold ? DidHitRelevantRepaintedObjectsAreaThreshold : 0);
}

WebLayoutMilestones kitLayoutMilestones(LayoutMilestones milestones)
{
    return (milestones & DidFirstLayout ? WebDidFirstLayout : 0)
        | (milestones & DidFirstVisuallyNonEmptyLayout ? WebDidFirstVisuallyNonEmptyLayout : 0)
        | (milestones & DidHitRelevantRepaintedObjectsAreaThreshold ? WebDidHitRelevantRepaintedObjectsAreaThreshold : 0);
}

static WebPageVisibilityState kit(PageVisibilityState visibilityState)
{
    switch (visibilityState) {
    case PageVisibilityStateVisible:
        return WebPageVisibilityStateVisible;
    case PageVisibilityStateHidden:
        return WebPageVisibilityStateHidden;
    case PageVisibilityStatePrerender:
        return WebPageVisibilityStatePrerender;
    }

    ASSERT_NOT_REACHED();
    return WebPageVisibilityStateVisible;
}

@interface WebView (WebFileInternal)
#if !PLATFORM(IOS)
- (float)_deviceScaleFactor;
#endif
- (BOOL)_isLoading;
- (WebFrameView *)_frameViewAtWindowPoint:(NSPoint)point;
- (WebFrame *)_focusedFrame;
+ (void)_preflightSpellChecker;
- (BOOL)_continuousCheckingAllowed;
- (NSResponder *)_responderForResponderOperations;
@end

NSString *WebElementDOMNodeKey =            @"WebElementDOMNode";
NSString *WebElementFrameKey =              @"WebElementFrame";
NSString *WebElementImageKey =              @"WebElementImage";
NSString *WebElementImageAltStringKey =     @"WebElementImageAltString";
NSString *WebElementImageRectKey =          @"WebElementImageRect";
NSString *WebElementImageURLKey =           @"WebElementImageURL";
NSString *WebElementIsSelectedKey =         @"WebElementIsSelected";
NSString *WebElementLinkLabelKey =          @"WebElementLinkLabel";
NSString *WebElementLinkTargetFrameKey =    @"WebElementTargetFrame";
NSString *WebElementLinkTitleKey =          @"WebElementLinkTitle";
NSString *WebElementLinkURLKey =            @"WebElementLinkURL";
NSString *WebElementMediaURLKey =           @"WebElementMediaURL";
NSString *WebElementSpellingToolTipKey =    @"WebElementSpellingToolTip";
NSString *WebElementTitleKey =              @"WebElementTitle";
NSString *WebElementLinkIsLiveKey =         @"WebElementLinkIsLive";
NSString *WebElementIsInScrollBarKey =      @"WebElementIsInScrollBar";
NSString *WebElementIsContentEditableKey =  @"WebElementIsContentEditableKey";

NSString *WebViewProgressStartedNotification =          @"WebProgressStartedNotification";
NSString *WebViewProgressEstimateChangedNotification =  @"WebProgressEstimateChangedNotification";
#if !PLATFORM(IOS)
NSString *WebViewProgressFinishedNotification =         @"WebProgressFinishedNotification";
#else
NSString * const WebViewProgressEstimatedProgressKey = @"WebProgressEstimatedProgressKey";
NSString * const WebViewProgressBackgroundColorKey =   @"WebProgressBackgroundColorKey";
#endif

NSString * const WebViewDidBeginEditingNotification =         @"WebViewDidBeginEditingNotification";
NSString * const WebViewDidChangeNotification =               @"WebViewDidChangeNotification";
NSString * const WebViewDidEndEditingNotification =           @"WebViewDidEndEditingNotification";
NSString * const WebViewDidChangeTypingStyleNotification =    @"WebViewDidChangeTypingStyleNotification";
NSString * const WebViewDidChangeSelectionNotification =      @"WebViewDidChangeSelectionNotification";

enum { WebViewVersion = 4 };

#define timedLayoutSize 4096

static NSMutableSet *schemesWithRepresentationsSet;

#if !PLATFORM(IOS)
NSString *_WebCanGoBackKey =            @"canGoBack";
NSString *_WebCanGoForwardKey =         @"canGoForward";
NSString *_WebEstimatedProgressKey =    @"estimatedProgress";
NSString *_WebIsLoadingKey =            @"isLoading";
NSString *_WebMainFrameIconKey =        @"mainFrameIcon";
NSString *_WebMainFrameTitleKey =       @"mainFrameTitle";
NSString *_WebMainFrameURLKey =         @"mainFrameURL";
NSString *_WebMainFrameDocumentKey =    @"mainFrameDocument";
#endif

#if USE(QUICK_LOOK)
NSString *WebQuickLookFileNameKey = @"WebQuickLookFileNameKey";
NSString *WebQuickLookUTIKey      = @"WebQuickLookUTIKey";
#endif

NSString *_WebViewDidStartAcceleratedCompositingNotification = @"_WebViewDidStartAcceleratedCompositing";
NSString * const WebViewWillCloseNotification = @"WebViewWillCloseNotification";

#if ENABLE(REMOTE_INSPECTOR)
// FIXME: Legacy, remove this, switch to something from JavaScriptCore Inspector::RemoteInspectorServer.
NSString *_WebViewRemoteInspectorHasSessionChangedNotification = @"_WebViewRemoteInspectorHasSessionChangedNotification";
#endif

NSString *WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey = @"WebKitKerningAndLigaturesEnabledByDefault";

@interface WebProgressItem : NSObject
{
@public
    long long bytesReceived;
    long long estimatedLength;
}
@end

@implementation WebProgressItem
@end

static BOOL continuousSpellCheckingEnabled;
#if !PLATFORM(IOS)
static BOOL grammarCheckingEnabled;
static BOOL automaticQuoteSubstitutionEnabled;
static BOOL automaticLinkDetectionEnabled;
static BOOL automaticDashSubstitutionEnabled;
static BOOL automaticTextReplacementEnabled;
static BOOL automaticSpellingCorrectionEnabled;
#endif

@implementation WebView (AllWebViews)

static CFSetCallBacks NonRetainingSetCallbacks = {
    0,
    NULL,
    NULL,
    CFCopyDescription,
    CFEqual,
    CFHash
};

static CFMutableSetRef allWebViewsSet;

+ (void)_makeAllWebViewsPerformSelector:(SEL)selector
{
    if (!allWebViewsSet)
        return;

    [(NSMutableSet *)allWebViewsSet makeObjectsPerformSelector:selector];
}

- (void)_removeFromAllWebViewsSet
{
    if (allWebViewsSet)
        CFSetRemoveValue(allWebViewsSet, self);
}

- (void)_addToAllWebViewsSet
{
    if (!allWebViewsSet)
        allWebViewsSet = CFSetCreateMutable(NULL, 0, &NonRetainingSetCallbacks);

    CFSetSetValue(allWebViewsSet, self);
}

@end

@implementation WebView (WebPrivate)

static String webKitBundleVersionString()
{
    return [[NSBundle bundleForClass:[WebView class]] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
}

+ (NSString *)_standardUserAgentWithApplicationName:(NSString *)applicationName
{
    return standardUserAgentWithApplicationName(applicationName, webKitBundleVersionString());
}

#if PLATFORM(IOS)
- (void)_setBrowserUserAgentProductVersion:(NSString *)productVersion buildVersion:(NSString *)buildVersion bundleVersion:(NSString *)bundleVersion
{
    [self setApplicationNameForUserAgent:[NSString stringWithFormat:@"Version/%@ Mobile/%@ Safari/%@", productVersion, buildVersion, bundleVersion]];
}

- (void)_setUIWebViewUserAgentWithBuildVersion:(NSString *)buildVersion
{
    [self setApplicationNameForUserAgent:[@"Mobile/" stringByAppendingString:buildVersion]];
}
#endif // PLATFORM(IOS)

+ (void)_reportException:(JSValueRef)exception inContext:(JSContextRef)context
{
    if (!exception || !context)
        return;

    JSC::ExecState* execState = toJS(context);
    JSLockHolder lock(execState);

    // Make sure the context has a DOMWindow global object, otherwise this context didn't originate from a WebView.
    if (!toJSDOMWindow(execState->lexicalGlobalObject()))
        return;

    reportException(execState, toJS(execState, exception));
}

static void WebKitInitializeApplicationCachePathIfNecessary()
{
    static BOOL initialized = NO;
    if (initialized)
        return;

    NSString *appName = [[NSBundle mainBundle] bundleIdentifier];
    if (!appName)
        appName = [[NSProcessInfo processInfo] processName];
#if PLATFORM(IOS)
    if (WebCore::applicationIsMobileSafari() || WebCore::applicationIsWebApp())
        appName = @"com.apple.WebAppCache";
#endif

    ASSERT(appName);

    NSString* cacheDir = [NSString _webkit_localCacheDirectoryWithBundleIdentifier:appName];

    ApplicationCacheStorage::singleton().setCacheDirectory(cacheDir);
    initialized = YES;
}

static bool shouldEnableLoadDeferring()
{
    return !applicationIsAdobeInstaller();
}

static bool shouldRestrictWindowFocus()
{
#if PLATFORM(IOS)
    return true;
#else
    return !applicationIsHRBlock();
#endif
}

- (void)_dispatchPendingLoadRequests
{
    resourceLoadScheduler()->servePendingRequests();
}

#if !PLATFORM(IOS)
- (void)_registerDraggedTypes
{
    NSArray *editableTypes = [WebHTMLView _insertablePasteboardTypes];
    NSArray *URLTypes = [NSPasteboard _web_dragTypesForURL];
    NSMutableSet *types = [[NSMutableSet alloc] initWithArray:editableTypes];
    [types addObjectsFromArray:URLTypes];
    [self registerForDraggedTypes:[types allObjects]];
    [types release];
}

static bool needsOutlookQuirksScript()
{
    static bool isOutlookNeedingQuirksScript = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER)
        && applicationIsMicrosoftOutlook();
    return isOutlookNeedingQuirksScript;
}

static NSString *leakOutlookQuirksUserScriptContents()
{
    NSString *scriptPath = [[NSBundle bundleForClass:[WebView class]] pathForResource:@"OutlookQuirksUserScript" ofType:@"js"];
    NSStringEncoding encoding;
    return [[NSString alloc] initWithContentsOfFile:scriptPath usedEncoding:&encoding error:0];
}

-(void)_injectOutlookQuirksScript
{
    static NSString *outlookQuirksScriptContents = leakOutlookQuirksUserScriptContents();
    _private->group->userContentController().addUserScript(*core([WebScriptWorld world]), std::make_unique<UserScript>(outlookQuirksScriptContents, URL(), Vector<String>(), Vector<String>(), InjectAtDocumentEnd, InjectInAllFrames));

}
#endif

static bool shouldRespectPriorityInCSSAttributeSetters()
{
#if PLATFORM(IOS)
    static bool isStanzaNeedingAttributeSetterQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_CSS_ATTRIBUTE_SETTERS_IGNORING_PRIORITY)
        && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.lexcycle.stanza"];
    return isStanzaNeedingAttributeSetterQuirk;
#else
    static bool isIAdProducerNeedingAttributeSetterQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_CSS_ATTRIBUTE_SETTERS_IGNORING_PRIORITY)
        && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.iAdProducer"];
    return isIAdProducerNeedingAttributeSetterQuirk;
#endif
}

#if PLATFORM(IOS)
static bool shouldTransformsAffectOverflow()
{
    static bool shouldTransformsAffectOverflow = !applicationIsOkCupid() || WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_CSS_TRANSFORMS_AFFECTING_OVERFLOW);
    return shouldTransformsAffectOverflow;
}

static bool shouldDispatchJavaScriptWindowOnErrorEvents()
{
    static bool shouldDispatchJavaScriptWindowOnErrorEvents = !applicationIsFacebook() || WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_WINDOW_ON_ERROR);
    return shouldDispatchJavaScriptWindowOnErrorEvents;
}

static bool isInternalInstall()
{
    static bool isInternal = MGGetBoolAnswer(kMGQAppleInternalInstallCapability);
    return isInternal;
}

static bool didOneTimeInitialization = false;
#endif

static bool shouldUseLegacyBackgroundSizeShorthandBehavior()
{
#if PLATFORM(IOS)
    static bool shouldUseLegacyBackgroundSizeShorthandBehavior = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_LEGACY_BACKGROUNDSIZE_SHORTHAND_BEHAVIOR);
#else
    static bool shouldUseLegacyBackgroundSizeShorthandBehavior = applicationIsVersions()
        && !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_LEGACY_BACKGROUNDSIZE_SHORTHAND_BEHAVIOR);
#endif
    return shouldUseLegacyBackgroundSizeShorthandBehavior;
}

#if ENABLE(GAMEPAD)
static void WebKitInitializeGamepadProviderIfNecessary()
{
    static bool initialized = false;
    if (initialized)
        return;

    GamepadProvider::singleton().setSharedProvider(HIDGamepadProvider::singleton());
    initialized = true;
}
#endif

- (void)_commonInitializationWithFrameName:(NSString *)frameName groupName:(NSString *)groupName
{
    WebCoreThreadViolationCheckRoundTwo();

#ifndef NDEBUG
    WTF::RefCountedLeakCounter::suppressMessages(webViewIsOpen);
#endif
    
    WebPreferences *standardPreferences = [WebPreferences standardPreferences];
    [standardPreferences willAddToWebView];

    _private->preferences = [standardPreferences retain];
    _private->mainFrameDocumentReady = NO;
    _private->drawsBackground = YES;
#if !PLATFORM(IOS)
    _private->backgroundColor = [[NSColor colorWithDeviceWhite:1 alpha:1] retain];
#else
    _private->backgroundColor = CGColorRetain(cachedCGColor(Color::white, ColorSpaceDeviceRGB));
#endif
    _private->includesFlattenedCompositingLayersWhenDrawingToBitmap = YES;

#if PLATFORM(MAC)
    _private->windowVisibilityObserver = adoptNS([[WebWindowVisibilityObserver alloc] initWithView:self]);
#endif

    NSRect f = [self frame];
    WebFrameView *frameView = [[WebFrameView alloc] initWithFrame: NSMakeRect(0,0,f.size.width,f.size.height)];
    [frameView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:frameView];
    [frameView release];

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    if ([self respondsToSelector:@selector(_setActionMenu:)]) {
        RetainPtr<NSMenu> actionMenu = adoptNS([[NSMenu alloc] init]);
        self._actionMenu = actionMenu.get();
        _private->actionMenuController = [[WebActionMenuController alloc] initWithWebView:self];
        self._actionMenu.autoenablesItems = NO;
    }

    if (Class gestureClass = NSClassFromString(@"NSImmediateActionGestureRecognizer")) {
        RetainPtr<NSImmediateActionGestureRecognizer> recognizer = adoptNS([(NSImmediateActionGestureRecognizer *)[gestureClass alloc] initWithTarget:nil action:NULL]);
        _private->immediateActionController = [[WebImmediateActionController alloc] initWithWebView:self recognizer:recognizer.get()];
        [recognizer setDelegate:_private->immediateActionController];
    }
#endif

#if !PLATFORM(IOS)
    static bool didOneTimeInitialization = false;
#endif
    if (!didOneTimeInitialization) {
#if !LOG_DISABLED
        WebKitInitializeLoggingChannelsIfNecessary();
        WebCore::initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED

        // Initialize our platform strategies first before invoking the rest
        // of the initialization code which may depend on the strategies.
        WebPlatformStrategies::initializeIfNecessary();

#if PLATFORM(IOS)
        // Set the WebSQLiteDatabaseTrackerClient.
        SQLiteDatabaseTracker::setClient(WebSQLiteDatabaseTrackerClient::sharedWebSQLiteDatabaseTrackerClient());

        if ([standardPreferences databasesEnabled])
#endif
        [WebDatabaseManager sharedWebDatabaseManager];

#if PLATFORM(IOS)        
        if ([standardPreferences storageTrackerEnabled])
#endif
        WebKitInitializeStorageIfNecessary();
        WebKitInitializeApplicationCachePathIfNecessary();
#if ENABLE(GAMEPAD)
        WebKitInitializeGamepadProviderIfNecessary();
#endif
        
        Settings::setShouldRespectPriorityInCSSAttributeSetters(shouldRespectPriorityInCSSAttributeSetters());

#if PLATFORM(IOS)
        if (applicationIsMobileSafari())
            Settings::setShouldManageAudioSessionCategory(true);
#endif

        didOneTimeInitialization = true;
    }

    _private->group = WebViewGroup::getOrCreate(groupName, _private->preferences._localStorageDatabasePath);
    _private->group->addWebView(self);

    PageConfiguration pageConfiguration;
#if !PLATFORM(IOS)
    pageConfiguration.chromeClient = new WebChromeClient(self);
    pageConfiguration.contextMenuClient = new WebContextMenuClient(self);
#if ENABLE(DRAG_SUPPORT)
    pageConfiguration.dragClient = new WebDragClient(self);
#endif
    pageConfiguration.inspectorClient = new WebInspectorClient(self);
#else
    pageConfiguration.chromeClient = new WebChromeClientIOS(self);
    pageConfiguration.inspectorClient = new WebInspectorClient(self);
#endif
    pageConfiguration.editorClient = new WebEditorClient(self);
    pageConfiguration.alternativeTextClient = new WebAlternativeTextClient(self);
    pageConfiguration.loaderClientForMainFrame = new WebFrameLoaderClient;
    pageConfiguration.progressTrackerClient = new WebProgressTrackerClient(self);
    pageConfiguration.databaseProvider = &WebDatabaseProvider::singleton();
    pageConfiguration.storageNamespaceProvider = &_private->group->storageNamespaceProvider();
    pageConfiguration.userContentController = &_private->group->userContentController();
    pageConfiguration.visitedLinkStore = &_private->group->visitedLinkStore();
    _private->page = new Page(pageConfiguration);

    _private->page->setGroupName(groupName);

#if ENABLE(GEOLOCATION)
    WebCore::provideGeolocationTo(_private->page, new WebGeolocationClient(self));
#endif
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebCore::provideNotification(_private->page, new WebNotificationClient(self));
#endif
#if ENABLE(DEVICE_ORIENTATION)
#if !PLATFORM(IOS)
    WebCore::provideDeviceOrientationTo(_private->page, new WebDeviceOrientationClient(self));
#endif
#endif
#if ENABLE(MEDIA_STREAM)
    WebCore::provideUserMediaTo(_private->page, new WebUserMediaClient(self));
#endif

#if ENABLE(REMOTE_INSPECTOR)
    _private->page->setRemoteInspectionAllowed(true);
#endif

    _private->page->setCanStartMedia([self window]);
    _private->page->settings().setLocalStorageDatabasePath([[self preferences] _localStorageDatabasePath]);
    _private->page->settings().setUseLegacyBackgroundSizeShorthandBehavior(shouldUseLegacyBackgroundSizeShorthandBehavior());

#if !PLATFORM(IOS)
    if (needsOutlookQuirksScript()) {
        _private->page->settings().setShouldInjectUserScriptsInInitialEmptyDocument(true);
        [self _injectOutlookQuirksScript];
    }
#endif

#if PLATFORM(IOS)
    // Preserve the behavior we had before <rdar://problem/7580867>
    // by enforcing a 5MB limit for session storage.
    _private->page->settings().setSessionStorageQuota(5 * 1024 * 1024);

    [self _updateScreenScaleFromWindow];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_wakWindowScreenScaleChanged:) name:WAKWindowScreenScaleDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_wakWindowVisibilityChanged:) name:WAKWindowVisibilityDidChangeNotification object:nil];
    _private->_fixedPositionContent = [[WebFixedPositionContent alloc] initWithWebView:self];
#endif

    if ([[NSUserDefaults standardUserDefaults] objectForKey:WebSmartInsertDeleteEnabled])
        [self setSmartInsertDeleteEnabled:[[NSUserDefaults standardUserDefaults] boolForKey:WebSmartInsertDeleteEnabled]];

    [WebFrame _createMainFrameWithPage:_private->page frameName:frameName frameView:frameView];

#if !PLATFORM(IOS)
    NSRunLoop *runLoop = [NSRunLoop mainRunLoop];

    if (WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_LOADING_DURING_COMMON_RUNLOOP_MODES))
        [self scheduleInRunLoop:runLoop forMode:(NSString *)kCFRunLoopCommonModes];
    else
        [self scheduleInRunLoop:runLoop forMode:NSDefaultRunLoopMode];
#endif

    [self _addToAllWebViewsSet];
    
    // If there's already a next key view (e.g., from a nib), wire it up to our
    // contained frame view. In any case, wire our next key view up to the our
    // contained frame view. This works together with our becomeFirstResponder 
    // and setNextKeyView overrides.
    NSView *nextKeyView = [self nextKeyView];
    if (nextKeyView && nextKeyView != frameView)
        [frameView setNextKeyView:nextKeyView];
    [super setNextKeyView:frameView];

    if ([[self class] shouldIncludeInWebKitStatistics])
        ++WebViewCount;

#if !PLATFORM(IOS)
    [self _registerDraggedTypes];
#endif

    [self _setIsVisible:[self _isViewVisible]];

    WebPreferences *prefs = [self preferences];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesChangedNotification:)
                                                 name:WebPreferencesChangedInternalNotification object:prefs];

#if !PLATFORM(IOS)
    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];
#else
    // do this on the current thread on iOS, since the web thread could be blocked on the main thread,
    // and prefs need to be changed synchronously <rdar://problem/5841558>
    [self _preferencesChanged:prefs];
    _private->page->settings().setFontFallbackPrefersPictographs(true);
#endif

    MemoryPressureHandler::singleton().install();

    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_LOCAL_RESOURCE_SECURITY_RESTRICTION)) {
        // Originally, we allowed all local loads.
        SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForAll);
    } else if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_MORE_STRICT_LOCAL_RESOURCE_SECURITY_RESTRICTION)) {
        // Later, we allowed local loads for local URLs and documents loaded
        // with substitute data.
        SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForLocalAndSubstituteData);
    }

#if PLATFORM(MAC)
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_CONTENT_SNIFFING_FOR_FILE_URLS))
        ResourceHandle::forceContentSniffing();

    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
#endif
}

- (id)_initWithFrame:(NSRect)f frameName:(NSString *)frameName groupName:(NSString *)groupName
{
    self = [super initWithFrame:f];
    if (!self)
        return nil;

#ifdef ENABLE_WEBKIT_UNSET_DYLD_FRAMEWORK_PATH
    // DYLD_FRAMEWORK_PATH is used so Safari will load the development version of WebKit, which
    // may not work with other WebKit applications.  Unsetting DYLD_FRAMEWORK_PATH removes the
    // need for Safari to unset it to prevent it from being passed to applications it launches.
    // Unsetting it when a WebView is first created is as good a place as any.
    // See <http://bugs.webkit.org/show_bug.cgi?id=4286> for more details.
    if (getenv("WEBKIT_UNSET_DYLD_FRAMEWORK_PATH")) {
        unsetenv("DYLD_FRAMEWORK_PATH");
        unsetenv("WEBKIT_UNSET_DYLD_FRAMEWORK_PATH");
    }
#endif

    _private = [[WebViewPrivate alloc] init];
    [self _commonInitializationWithFrameName:frameName groupName:groupName];
    [self setMaintainsBackForwardList: YES];
    return self;
}

- (void)_viewWillDrawInternal
{
    Frame* frame = [self _mainCoreFrame];
    if (frame && frame->view())
        frame->view()->updateLayoutAndStyleIfNeededRecursive();
}

+ (NSArray *)_supportedMIMETypes
{
    // Load the plug-in DB allowing plug-ins to install types.
    [WebPluginDatabase sharedDatabase];
    return [[WebFrameView _viewTypesAllowImageTypeOmission:NO] allKeys];
}

#if !PLATFORM(IOS)
+ (NSArray *)_supportedFileExtensions
{
    NSMutableSet *extensions = [[NSMutableSet alloc] init];
    NSArray *MIMETypes = [self _supportedMIMETypes];
    NSEnumerator *enumerator = [MIMETypes objectEnumerator];
    NSString *MIMEType;
    while ((MIMEType = [enumerator nextObject]) != nil) {
        NSArray *extensionsForType = [[NSURLFileTypeMappings sharedMappings] extensionsForMIMEType:MIMEType];
        if (extensionsForType) {
            [extensions addObjectsFromArray:extensionsForType];
        }
    }
    NSArray *uniqueExtensions = [extensions allObjects];
    [extensions release];
    return uniqueExtensions;
}
#endif

#if PLATFORM(IOS)
+ (void)enableWebThread
{
    static BOOL isWebThreadEnabled = NO;
    if (!isWebThreadEnabled) {
        WebCoreObjCDeallocOnWebThread([WebBasePluginPackage class]);
        WebCoreObjCDeallocOnWebThread([WebDataSource class]);
        WebCoreObjCDeallocOnWebThread([WebFrame class]);
        WebCoreObjCDeallocOnWebThread([WebHTMLView class]);
        WebCoreObjCDeallocOnWebThread([WebHistoryItem class]);
        WebCoreObjCDeallocOnWebThread([WebPlainWhiteView class]);
        WebCoreObjCDeallocOnWebThread([WebPolicyDecisionListener class]);
        WebCoreObjCDeallocOnWebThread([WebView class]);
        WebCoreObjCDeallocOnWebThread([WebVisiblePosition class]);
        if (applicationIsTheEconomistOnIPhone() && !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_DELEGATE_CALLS_IN_COMMON_RUNLOOP_MODES))
            WebThreadSetDelegateSourceRunLoopMode(kCFRunLoopDefaultMode);
        WebThreadEnable();
        isWebThreadEnabled = YES;
    }
}

- (id)initSimpleHTMLDocumentWithStyle:(NSString *)style frame:(CGRect)frame preferences:(WebPreferences *)preferences groupName:(NSString *)groupName
{
    self = [super initWithFrame:frame];
    if (!self)
        return nil;
    
    _private = [[WebViewPrivate alloc] init];
    
#ifndef NDEBUG
    WTF::RefCountedLeakCounter::suppressMessages(webViewIsOpen);
#endif
    
    if (!preferences)
        preferences = [WebPreferences standardPreferences];
    [preferences willAddToWebView];
    
    _private->preferences = [preferences retain];
    _private->mainFrameDocumentReady = NO;
    _private->drawsBackground = YES;
    _private->backgroundColor = CGColorRetain(cachedCGColor(Color::white, ColorSpaceDeviceRGB));

    WebFrameView *frameView = nil;
    frameView = [[WebFrameView alloc] initWithFrame: CGRectMake(0,0,frame.size.width,frame.size.height)];
    [frameView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:frameView];
    [frameView release];

    _private->group = WebViewGroup::getOrCreate(groupName, _private->preferences._localStorageDatabasePath);
    _private->group->addWebView(self);

    PageConfiguration pageConfiguration;
    pageConfiguration.chromeClient = new WebChromeClientIOS(self);
#if ENABLE(DRAG_SUPPORT)
    pageConfiguration.dragClient = new WebDragClient(self);
#endif
    pageConfiguration.editorClient = new WebEditorClient(self);
    pageConfiguration.inspectorClient = new WebInspectorClient(self);
    pageConfiguration.loaderClientForMainFrame = new WebFrameLoaderClient;
    pageConfiguration.progressTrackerClient = new WebProgressTrackerClient(self);
    pageConfiguration.databaseProvider = &WebDatabaseProvider::singleton();
    pageConfiguration.storageNamespaceProvider = &_private->group->storageNamespaceProvider();
    pageConfiguration.userContentController = &_private->group->userContentController();
    pageConfiguration.visitedLinkStore = &_private->group->visitedLinkStore();

    _private->page = new Page(pageConfiguration);
    
    [self setSmartInsertDeleteEnabled:YES];
    
    // FIXME: <rdar://problem/6851451> Should respect preferences in fast path WebView initialization
    // We are ignoring the preferences object on fast path and just using Settings defaults (everything fancy off).
    // This matches how UIKit sets up the preferences. We need to set  default values for fonts though, <rdar://problem/6850611>.
    // This should be revisited later. There is some risk involved, _preferencesChanged used to show up badly in Shark.
    _private->page->settings().setMinimumLogicalFontSize(9);
    _private->page->settings().setDefaultFontSize([_private->preferences defaultFontSize]);
    _private->page->settings().setDefaultFixedFontSize(13);
    _private->page->settings().setDownloadableBinaryFontsEnabled(false);
    _private->page->settings().setAcceleratedDrawingEnabled([preferences acceleratedDrawingEnabled]);
    
    _private->page->settings().setFontFallbackPrefersPictographs(true);
    _private->page->settings().setPictographFontFamily("AppleColorEmoji");
    
    // FIXME: this is a workaround for <rdar://problem/11518688> REGRESSION: Quoted text font changes when replying to certain email
    _private->page->settings().setStandardFontFamily([_private->preferences standardFontFamily]);
    
    // FIXME: this is a workaround for <rdar://problem/11820090> Quoted text changes in size when replying to certain email
    _private->page->settings().setMinimumFontSize([_private->preferences minimumFontSize]);

    _private->page->setGroupName(groupName);

#if ENABLE(REMOTE_INSPECTOR)
    _private->page->setRemoteInspectionAllowed(isInternalInstall());
#endif
    
    [self _updateScreenScaleFromWindow];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_wakWindowScreenScaleChanged:) name:WAKWindowScreenScaleDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_wakWindowVisibilityChanged:) name:WAKWindowVisibilityDidChangeNotification object:nil];

    [WebFrame _createMainFrameWithSimpleHTMLDocumentWithPage:_private->page frameView:frameView style:style];
    
    [self _addToAllWebViewsSet];
    
    ++WebViewCount;
    
    SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForLocalAndSubstituteData);
    
    return self;
}

+ (void)_handleMemoryWarning
{
    ASSERT(WebThreadIsCurrent());
    WebKit::MemoryMeasure totalMemory("Memory warning: Overall memory change from [WebView _handleMemoryWarning].");

    // Always peform the following.
    [WebView purgeInactiveFontData];

    // Only perform the remaining if a non-simple document was created.
    if (!didOneTimeInitialization)
        return;

    tileControllerMemoryHandler().trimUnparentedTilesToTarget(0);

    [WebStorageManager closeIdleLocalStorageDatabases];
    StorageThread::releaseFastMallocFreeMemoryInAllThreads();

    [WebView _releaseMemoryNow];
}

+ (void)registerForMemoryNotifications
{
    BOOL shouldAutoClearPressureOnMemoryRelease = !WebCore::applicationIsMobileSafari();

    MemoryPressureHandler::singleton().installMemoryReleaseBlock(^{
        [WebView _handleMemoryWarning];
    }, shouldAutoClearPressureOnMemoryRelease);

    static dispatch_source_t memoryNotificationEventSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYSTATUS, 0, DISPATCH_MEMORYSTATUS_PRESSURE_WARN, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    dispatch_source_set_event_handler(memoryNotificationEventSource, ^{
        // Set memory pressure flag and schedule releasing memory in web thread runloop exit.
        MemoryPressureHandler::singleton().setReceivedMemoryPressure(WebCore::MemoryPressureReasonVMPressure);
    });

    dispatch_resume(memoryNotificationEventSource);

    if (!shouldAutoClearPressureOnMemoryRelease) {
        // Listen to memory status notification to reset the memory pressure flag.
        static dispatch_source_t memoryStatusEventSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYSTATUS,
                                                                                    0,
                                                                                    DISPATCH_MEMORYSTATUS_PRESSURE_WARN | DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL,
                                                                                    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
        dispatch_source_set_event_handler(memoryStatusEventSource, ^{
            unsigned long currentStatus = dispatch_source_get_data(memoryStatusEventSource);
            if (currentStatus == DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL)
                MemoryPressureHandler::singleton().clearMemoryPressure();
            else if (currentStatus == DISPATCH_MEMORYSTATUS_PRESSURE_WARN)
                MemoryPressureHandler::singleton().setReceivedMemoryPressure(WebCore::MemoryPressureReasonVMStatus);
        });

        dispatch_resume(memoryStatusEventSource);
    }
}

+ (void)_releaseMemoryNow
{
    ASSERT(WebThreadIsCurrent());
    [WebView discardAllCompiledCode];
    [WebView garbageCollectNow];
    [WebView purgeInactiveFontData];
    [WebView drainLayerPool];
    [WebCache emptyInMemoryResources];
    [WebView releaseFastMallocMemoryOnCurrentThread];

    dispatch_async(dispatch_get_main_queue(), ^{
        // Clear the main thread's TCMalloc thread cache.
        [WebView releaseFastMallocMemoryOnCurrentThread];
    });
}

+ (void)_clearPrivateBrowsingSessionCookieStorage
{
    WebFrameNetworkingContext::clearPrivateBrowsingSessionCookieStorage();
}

- (void)_replaceCurrentHistoryItem:(WebHistoryItem *)item
{
    Frame* frame = [self _mainCoreFrame];
    if (frame)
        frame->loader().history().replaceCurrentItem(core(item));
}

+ (void)willEnterBackgroundWithCompletionHandler:(void(^)(void))handler
{
    WebThreadRun(^{
        [WebView _releaseMemoryNow];
        dispatch_async(dispatch_get_main_queue(), handler);
    });
}

+ (void)releaseFastMallocMemoryOnCurrentThread
{
    WebKit::MemoryMeasure measurer("Memory warning: Releasing fast malloc memory to system.");
    WTF::releaseFastMallocFreeMemory();
}

+ (void)garbageCollectNow
{
    ASSERT(WebThreadIsCurrent());
    WebKit::MemoryMeasure measurer("Memory warning: Calling JavaScript GC.");
    gcController().garbageCollectNow();
}

+ (void)purgeInactiveFontData
{
    ASSERT(WebThreadIsCurrent());
    WebKit::MemoryMeasure measurer("Memory warning: Purging inactive font data.");
    FontCache::singleton().purgeInactiveFontData();
}

+ (void)drainLayerPool
{
    ASSERT(WebThreadIsCurrent());
    WebKit::MemoryMeasure measurer("Memory warning: Draining layer pool.");
    WebCore::LegacyTileCache::drainLayerPool();
}

+ (void)discardAllCompiledCode
{
    ASSERT(WebThreadIsCurrent());
    WebKit::MemoryMeasure measurer("Memory warning: Discarding JIT'ed code.");
    gcController().discardAllCompiledCode();
}

+ (BOOL)isCharacterSmartReplaceExempt:(unichar)character isPreviousCharacter:(BOOL)b
{
    return WebCore::isCharacterSmartReplaceExempt(character, b);
}

- (void)updateLayoutIgnorePendingStyleSheets
{
    WebThreadRun(^{
        for (Frame* frame = [self _mainCoreFrame]; frame; frame = frame->tree().traverseNext()) {
            Document *document = frame->document();
            if (document)
                document->updateLayoutIgnorePendingStylesheets();
        }
    });
}
#endif // PLATFORM(IOS)

static NSMutableSet *knownPluginMIMETypes()
{
    static NSMutableSet *mimeTypes = [[NSMutableSet alloc] init];
    
    return mimeTypes;
}

+ (void)_registerPluginMIMEType:(NSString *)MIMEType
{
    [WebView registerViewClass:[WebHTMLView class] representationClass:[WebHTMLRepresentation class] forMIMEType:MIMEType];
    [knownPluginMIMETypes() addObject:MIMEType];
}

+ (void)_unregisterPluginMIMEType:(NSString *)MIMEType
{
    [self _unregisterViewClassAndRepresentationClassForMIMEType:MIMEType];
    [knownPluginMIMETypes() removeObject:MIMEType];
}

+ (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins
{
    MIMEType = [MIMEType lowercaseString];
    Class viewClass = [[WebFrameView _viewTypesAllowImageTypeOmission:YES] _webkit_objectForMIMEType:MIMEType];
    Class repClass = [[WebDataSource _repTypesAllowImageTypeOmission:YES] _webkit_objectForMIMEType:MIMEType];

#if PLATFORM(IOS)
#define WebPDFView ([WebView _getPDFViewClass])
#endif
    if (!viewClass || !repClass || [[WebPDFView supportedMIMETypes] containsObject:MIMEType]) {
#if PLATFORM(IOS)
#undef WebPDFView
#endif
        // Our optimization to avoid loading the plug-in DB and image types for the HTML case failed.

        if (allowPlugins) {
            // Load the plug-in DB allowing plug-ins to install types.
            [WebPluginDatabase sharedDatabase];
        }

        // Load the image types and get the view class and rep class. This should be the fullest picture of all handled types.
        viewClass = [[WebFrameView _viewTypesAllowImageTypeOmission:NO] _webkit_objectForMIMEType:MIMEType];
        repClass = [[WebDataSource _repTypesAllowImageTypeOmission:NO] _webkit_objectForMIMEType:MIMEType];
    }
    
    if (viewClass && repClass) {
        if (viewClass == [WebHTMLView class] && repClass == [WebHTMLRepresentation class]) {
            // Special-case WebHTMLView for text types that shouldn't be shown.
            if ([[WebHTMLView unsupportedTextMIMETypes] containsObject:MIMEType])
                return NO;

            // If the MIME type is a known plug-in we might not want to load it.
            if (!allowPlugins && [knownPluginMIMETypes() containsObject:MIMEType]) {
                BOOL isSupportedByWebKit = [[WebHTMLView supportedNonImageMIMETypes] containsObject:MIMEType] ||
                    [[WebHTMLView supportedMIMETypes] containsObject:MIMEType];
                
                // If this is a known plug-in MIME type and WebKit can't show it natively, we don't want to show it.
                if (!isSupportedByWebKit)
                    return NO;
            }
        }
        if (vClass)
            *vClass = viewClass;
        if (rClass)
            *rClass = repClass;
        return YES;
    }
    
    return NO;
}

- (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType
{
    if ([[self class] _viewClass:vClass andRepresentationClass:rClass forMIMEType:MIMEType allowingPlugins:[_private->preferences arePlugInsEnabled]])
        return YES;
#if !PLATFORM(IOS)
    if (_private->pluginDatabase) {
        WebBasePluginPackage *pluginPackage = [_private->pluginDatabase pluginForMIMEType:MIMEType];
        if (pluginPackage) {
            if (vClass)
                *vClass = [WebHTMLView class];
            if (rClass)
                *rClass = [WebHTMLRepresentation class];
            return YES;
        }
    }
#endif
    return NO;
}

+ (void)_setAlwaysUsesComplexTextCodePath:(BOOL)f
{
    FontCascade::setCodePath(f ? FontCascade::Complex : FontCascade::Auto);
}

+ (void)_setAllowsRoundingHacks:(BOOL)allowsRoundingHacks
{
    TextRun::setAllowsRoundingHacks(allowsRoundingHacks);
}

+ (BOOL)_allowsRoundingHacks
{
    return TextRun::allowsRoundingHacks();
}

+ (BOOL)canCloseAllWebViews
{
    return DOMWindow::dispatchAllPendingBeforeUnloadEvents();
}

+ (void)closeAllWebViews
{
    DOMWindow::dispatchAllPendingUnloadEvents();

    // This will close the WebViews in a random order. Change this if close order is important.
    // Make a new set to avoid mutating the set we are enumerating.
    NSSet *webViewsToClose = [NSSet setWithSet:(NSSet *)allWebViewsSet]; 
    NSEnumerator *enumerator = [webViewsToClose objectEnumerator];
    while (WebView *webView = [enumerator nextObject])
        [webView close];
}

+ (BOOL)canShowFile:(NSString *)path
{
    return [[self class] canShowMIMEType:[WebView _MIMETypeForFile:path]];
}

#if !PLATFORM(IOS)
+ (NSString *)suggestedFileExtensionForMIMEType:(NSString *)type
{
    return [[NSURLFileTypeMappings sharedMappings] preferredExtensionForMIMEType:type];
}
#endif

- (BOOL)_isClosed
{
    return !_private || _private->closed;
}

#if PLATFORM(IOS)
- (void)_dispatchUnloadEvent
{
    WebThreadRun(^{
        WebFrame *mainFrame = [self mainFrame];
        Frame *coreMainFrame = core(mainFrame);
        if (coreMainFrame) {
            Document *document = coreMainFrame->document();
            if (document)
                document->dispatchWindowEvent(Event::create(eventNames().unloadEvent, false, false));
        }
    });
}

- (DOMCSSStyleDeclaration *)styleAtSelectionStart
{
    WebFrame *mainFrame = [self mainFrame];
    Frame *coreMainFrame = core(mainFrame);
    if (!coreMainFrame)
        return nil;
    return coreMainFrame->styleAtSelectionStart();
}

- (NSUInteger)_renderTreeSize
{
    if (!_private->page)
        return 0;
    return _private->page->renderTreeSize();
}

- (void)_dispatchTileDidDraw:(CALayer*)tile
{
    id mailDelegate = [self _webMailDelegate];
    if ([mailDelegate respondsToSelector:@selector(_webthread_webView:tileDidDraw:)]) {
        [mailDelegate _webthread_webView:self tileDidDraw:tile];
        return;
    }

    if (!OSAtomicCompareAndSwap32(0, 1, &_private->didDrawTiles))
        return;

    WebThreadLock();

    [[[self _UIKitDelegateForwarder] asyncForwarder] webViewDidDrawTiles:self];
}

- (void)_willStartScrollingOrZooming
{
    // Pause timers during top level interaction
    if (_private->mainViewIsScrollingOrZooming)
        return;
    _private->mainViewIsScrollingOrZooming = YES;

    // This suspends active DOM objects like timers, but not media.
    [[self mainFrame] setTimeoutsPaused:YES];

    // This defers loading callbacks only.
    // WARNING: This behavior could change if Bug 49401 lands in open source WebKit again.
    [self setDefersCallbacks:YES];
}

- (void)_didFinishScrollingOrZooming
{
    if (!_private->mainViewIsScrollingOrZooming)
        return;
    _private->mainViewIsScrollingOrZooming = NO;
    [self setDefersCallbacks:NO];
    [[self mainFrame] setTimeoutsPaused:NO];
    if (FrameView* view = [self _mainCoreFrame]->view())
        view->resumeVisibleImageAnimationsIncludingSubframes();
}

- (void)_setResourceLoadSchedulerSuspended:(BOOL)suspend
{
    if (suspend)
        resourceLoadScheduler()->suspendPendingRequests();
    else
        resourceLoadScheduler()->resumePendingRequests();
}

+ (void)_setAllowCookies:(BOOL)allow
{
    ResourceRequestBase::setDefaultAllowCookies(allow);
}

+ (BOOL)_allowCookies
{
    return ResourceRequestBase::defaultAllowCookies();
}

+ (BOOL)_isUnderMemoryPressure
{
    return MemoryPressureHandler::singleton().isUnderMemoryPressure();
}

+ (void)_clearMemoryPressure
{
    MemoryPressureHandler::singleton().clearMemoryPressure();
}

+ (BOOL)_shouldWaitForMemoryClearMessage
{
    return MemoryPressureHandler::singleton().shouldWaitForMemoryClearMessage();
}
#endif // PLATFORM(IOS)

- (void)_closePluginDatabases
{
    pluginDatabaseClientCount--;

    // Close both sets of plug-in databases because plug-ins need an opportunity to clean up files, etc.

#if !PLATFORM(IOS)
    // Unload the WebView local plug-in database. 
    if (_private->pluginDatabase) {
        [_private->pluginDatabase destroyAllPluginInstanceViews];
        [_private->pluginDatabase close];
        [_private->pluginDatabase release];
        _private->pluginDatabase = nil;
    }
#endif
    
    // Keep the global plug-in database active until the app terminates to avoid having to reload plug-in bundles.
    if (!pluginDatabaseClientCount && applicationIsTerminating)
        [WebPluginDatabase closeSharedDatabase];
}

- (void)_closeWithFastTeardown 
{
#ifndef NDEBUG
    WTF::RefCountedLeakCounter::suppressMessages("At least one WebView was closed with fast teardown.");
#endif

#if !PLATFORM(IOS)
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
#endif
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [self _closePluginDatabases];
}

static bool fastDocumentTeardownEnabled()
{
#ifdef NDEBUG
    static bool enabled = ![[NSUserDefaults standardUserDefaults] boolForKey:WebKitEnableFullDocumentTeardownPreferenceKey];
#else
    static bool initialized = false;
    static bool enabled = false;
    if (!initialized) {
        // This allows debug builds to default to not have fast teardown, so leak checking still works.
        // But still allow the WebKitEnableFullDocumentTeardown default to override it if present.
        NSNumber *setting = [[NSUserDefaults standardUserDefaults] objectForKey:WebKitEnableFullDocumentTeardownPreferenceKey];
        if (setting)
            enabled = ![setting boolValue];
        initialized = true;
    }
#endif
    return enabled;
}

// _close is here only for backward compatibility; clients and subclasses should use
// public method -close instead.
- (void)_close
{
#if PLATFORM(IOS)
    // Always clear delegates on calling thread, becasue caller can deallocate delegates right after calling -close
    // (and could in fact already be in delegate's -dealloc method, as seen in <rdar://problem/11540972>).

    [self _clearDelegates];

    // Fix for problems such as <rdar://problem/5774587> Crash closing tab in WebCore::Frame::page() from -[WebCoreFrameBridge pauseTimeouts]
    WebThreadRun(^{
#endif            

    if (!_private || _private->closed)
        return;

    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewWillCloseNotification object:self];

    _private->closed = YES;
    [self _removeFromAllWebViewsSet];

#ifndef NDEBUG
    WTF::RefCountedLeakCounter::cancelMessageSuppression(webViewIsOpen);
#endif

    // To quit the apps fast we skip document teardown, except plugins
    // need to be destroyed and unloaded.
    if (applicationIsTerminating && fastDocumentTeardownEnabled()) {
        [self _closeWithFastTeardown];
        return;
    }

#if ENABLE(VIDEO) && !PLATFORM(IOS)
    [self _exitVideoFullscreen];
#endif

#if PLATFORM(IOS)
    _private->closing = YES;
#endif

    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->loader().detachFromParent();

    [self setHostWindow:nil];

#if !PLATFORM(IOS)
    [self setDownloadDelegate:nil];
    [self setEditingDelegate:nil];
    [self setFrameLoadDelegate:nil];
    [self setPolicyDelegate:nil];
    [self setResourceLoadDelegate:nil];
    [self setScriptDebugDelegate:nil];
    [self setUIDelegate:nil];

    [_private->inspector webViewClosed];
#endif
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    [_private->actionMenuController webViewClosed];
    [_private->immediateActionController webViewClosed];
#endif

#if !PLATFORM(IOS)
    // To avoid leaks, call removeDragCaret in case it wasn't called after moveDragCaretToPoint.
    [self removeDragCaret];
#endif

    _private->group->removeWebView(self);

    // Deleteing the WebCore::Page will clear the page cache so we call destroy on
    // all the plug-ins in the page cache to break any retain cycles.
    // See comment in HistoryItem::releaseAllPendingPageCaches() for more information.
    Page* page = _private->page;
    _private->page = 0;
    delete page;

#if !PLATFORM(IOS)
    if (_private->hasSpellCheckerDocumentTag) {
        [[NSSpellChecker sharedSpellChecker] closeSpellDocumentWithTag:_private->spellCheckerDocumentTag];
        _private->hasSpellCheckerDocumentTag = NO;
    }
#endif

    if (_private->layerFlushController) {
        _private->layerFlushController->invalidate();
        _private->layerFlushController = nullptr;
    }

    [[self _notificationProvider] unregisterWebView:self];

#if !PLATFORM(IOS)
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
#endif
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [WebPreferences _removeReferenceForIdentifier:[self preferencesIdentifier]];

    WebPreferences *preferences = _private->preferences;
    _private->preferences = nil;
    [preferences didRemoveFromWebView];
    [preferences release];

    [self _closePluginDatabases];

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (_private->m_playbackTargetPicker) {
        _private->m_playbackTargetPicker->invalidate();
        _private->m_playbackTargetPicker = nullptr;
    }
#endif

#ifndef NDEBUG
    // Need this to make leak messages accurate.
    if (applicationIsTerminating) {
        gcController().garbageCollectNow();
        [WebCache setDisabled:YES];
    }
#endif
#if PLATFORM(IOS)
    // Fix for problems such as <rdar://problem/5774587> Crash closing tab in WebCore::Frame::page() from -[WebCoreFrameBridge pauseTimeouts]
    });
#endif            
}

// Indicates if the WebView is in the midst of a user gesture.
- (BOOL)_isProcessingUserGesture
{
    return ScriptController::processingUserGesture();
}

+ (NSString *)_MIMETypeForFile:(NSString *)path
{
#if !PLATFORM(IOS)
    NSString *extension = [path pathExtension];
#endif
    NSString *MIMEType = nil;

#if !PLATFORM(IOS)
    // Get the MIME type from the extension.
    if ([extension length] != 0) {
        MIMEType = [[NSURLFileTypeMappings sharedMappings] MIMETypeForExtension:extension];
    }
#endif

    // If we can't get a known MIME type from the extension, sniff.
    if ([MIMEType length] == 0 || [MIMEType isEqualToString:@"application/octet-stream"]) {
        NSFileHandle *handle = [NSFileHandle fileHandleForReadingAtPath:path];
        NSData *data = [handle readDataOfLength:WEB_GUESS_MIME_TYPE_PEEK_LENGTH];
        [handle closeFile];
        if ([data length] != 0) {
            MIMEType = [data _webkit_guessedMIMEType];
        }
        if ([MIMEType length] == 0) {
            MIMEType = @"application/octet-stream";
        }
    }

    return MIMEType;
}

- (WebDownload *)_downloadURL:(NSURL *)URL
{
    ASSERT(URL);
    
    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:URL];
    WebDownload *download = [WebDownload _downloadWithRequest:request
                                                     delegate:_private->downloadDelegate
                                                    directory:nil];
    [request release];
    
    return download;
}

- (WebView *)_openNewWindowWithRequest:(NSURLRequest *)request
{
    NSDictionary *features = [[NSDictionary alloc] init];
    WebView *newWindowWebView = [[self _UIDelegateForwarder] webView:self
                                            createWebViewWithRequest:nil
                                                      windowFeatures:features];
    [features release];
    if (!newWindowWebView)
        return nil;

    CallUIDelegate(newWindowWebView, @selector(webViewShow:));
    return newWindowWebView;
}

- (WebInspector *)inspector
{
    if (!_private->inspector)
        _private->inspector = [[WebInspector alloc] initWithWebView:self];
    return _private->inspector;
}

#if ENABLE(REMOTE_INSPECTOR)
+ (void)_enableRemoteInspector
{
    RemoteInspector::singleton().start();
}

+ (void)_disableRemoteInspector
{
    RemoteInspector::singleton().stop();
}

+ (void)_disableAutoStartRemoteInspector
{
    RemoteInspector::startDisabled();
}

+ (BOOL)_isRemoteInspectorEnabled
{
    return RemoteInspector::singleton().enabled();
}

+ (BOOL)_hasRemoteInspectorSession
{
    return RemoteInspector::singleton().hasActiveDebugSession();
}

- (BOOL)allowsRemoteInspection
{
    return _private->page->remoteInspectionAllowed();
}

- (void)setAllowsRemoteInspection:(BOOL)allow
{
    _private->page->setRemoteInspectionAllowed(allow);
}

- (void)setShowingInspectorIndication:(BOOL)showing
{
#if PLATFORM(IOS)
    ASSERT(WebThreadIsLocked());

    if (showing) {
        if (!_private->indicateLayer) {
            _private->indicateLayer = [[WebIndicateLayer alloc] initWithWebView:self];
            [_private->indicateLayer setNeedsLayout];
            [[[self window] hostLayer] addSublayer:_private->indicateLayer];
        }
    } else {
        [_private->indicateLayer removeFromSuperlayer];
        [_private->indicateLayer release];
        _private->indicateLayer = nil;
    }
#else
    // Implemented in WebCore::InspectorOverlay.
#endif
}

#if PLATFORM(IOS)
- (void)_setHostApplicationProcessIdentifier:(pid_t)pid auditToken:(audit_token_t)auditToken
{
    RetainPtr<CFDataRef> auditData = adoptCF(CFDataCreate(nullptr, (const UInt8*)&auditToken, sizeof(auditToken)));
    RemoteInspector::singleton().setParentProcessInformation(pid, auditData);
}
#endif // PLATFORM(IOS)
#endif // ENABLE(REMOTE_INSPECTOR)

- (WebCore::Page*)page
{
    return _private->page;
}

#if !PLATFORM(IOS)
- (NSMenu *)_menuForElement:(NSDictionary *)element defaultItems:(NSArray *)items
{
    NSArray *defaultMenuItems = [[WebDefaultUIDelegate sharedUIDelegate] webView:self contextMenuItemsForElement:element defaultMenuItems:items];
    NSArray *menuItems = defaultMenuItems;

    // CallUIDelegate returns nil if UIDelegate is nil or doesn't respond to the selector. So we need to check that here
    // to distinguish between using defaultMenuItems or the delegate really returning nil to say "no context menu".
    SEL selector = @selector(webView:contextMenuItemsForElement:defaultMenuItems:);
    if (_private->UIDelegate && [_private->UIDelegate respondsToSelector:selector]) {
        menuItems = CallUIDelegate(self, selector, element, defaultMenuItems);
        if (!menuItems)
            return nil;
    }

    unsigned count = [menuItems count];
    if (!count)
        return nil;

    NSMenu *menu = [[NSMenu alloc] init];
    for (unsigned i = 0; i < count; i++)
        [menu addItem:[menuItems objectAtIndex:i]];

    return [menu autorelease];
}
#endif

- (void)_mouseDidMoveOverElement:(NSDictionary *)dictionary modifierFlags:(NSUInteger)modifierFlags
{
    // We originally intended to call this delegate method sometimes with a nil dictionary, but due to
    // a bug dating back to WebKit 1.0 this delegate was never called with nil! Unfortunately we can't
    // start calling this with nil since it will break Adobe Help Viewer, and possibly other clients.
    if (!dictionary)
        return;
    CallUIDelegate(self, @selector(webView:mouseDidMoveOverElement:modifierFlags:), dictionary, modifierFlags);
}

- (void)_loadBackForwardListFromOtherView:(WebView *)otherView
{
    if (!_private->page)
        return;
    
    if (!otherView->_private->page)
        return;
    
    // It turns out the right combination of behavior is done with the back/forward load
    // type.  (See behavior matrix at the top of WebFramePrivate.)  So we copy all the items
    // in the back forward list, and go to the current one.

    BackForwardClient* backForwardClient = _private->page->backForward().client();
    ASSERT(!backForwardClient->currentItem()); // destination list should be empty

    BackForwardClient* otherBackForwardClient = otherView->_private->page->backForward().client();
    if (!otherBackForwardClient->currentItem())
        return; // empty back forward list, bail
    
    HistoryItem* newItemToGoTo = nullptr;

    int lastItemIndex = otherBackForwardClient->forwardListCount();
    for (int i = -otherBackForwardClient->backListCount(); i <= lastItemIndex; ++i) {
        if (i == 0) {
            // If this item is showing , save away its current scroll and form state,
            // since that might have changed since loading and it is normally not saved
            // until we leave that page.
            otherView->_private->page->mainFrame().loader().history().saveDocumentAndScrollState();
        }
        Ref<HistoryItem> newItem = otherBackForwardClient->itemAtIndex(i)->copy();
        if (i == 0) 
            newItemToGoTo = newItem.ptr();
        backForwardClient->addItem(WTF::move(newItem));
    }
    
    ASSERT(newItemToGoTo);
    _private->page->goToItem(*newItemToGoTo, FrameLoadType::IndexedBackForward);
}

- (void)_setFormDelegate: (id<WebFormDelegate>)delegate
{
    _private->formDelegate = delegate;
#if PLATFORM(IOS)
    [_private->formDelegateForwarder clearTarget];
    [_private->formDelegateForwarder release];
    _private->formDelegateForwarder = nil;
#endif
}

- (id<WebFormDelegate>)_formDelegate
{
    return _private->formDelegate;
}

#if PLATFORM(IOS)
- (id)_formDelegateForwarder
{
    if (_private->closing)
        return nil;

    if (!_private->formDelegateForwarder)
        _private->formDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:[self _formDelegate] defaultTarget:[WebDefaultFormDelegate sharedFormDelegate]];
    return _private->formDelegateForwarder;
}

- (id)_formDelegateForSelector:(SEL)selector
{
    id delegate = self->_private->formDelegate;
    if ([delegate respondsToSelector:selector])
        return delegate;

    delegate = [WebDefaultFormDelegate sharedFormDelegate];
    if ([delegate respondsToSelector:selector])
        return delegate;

    return nil;
}
#endif

#if !PLATFORM(IOS)
- (BOOL)_needsAdobeFrameReloadingQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.adobe.Acrobat", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Acrobat.Pro", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Reader", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.distiller", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Contribute", -1, 4.2)
        || WKAppVersionCheckLessThan(@"com.adobe.dreamweaver-9.0", -1, 9.1)
        || WKAppVersionCheckLessThan(@"com.macromedia.fireworks", -1, 9.1)
        || WKAppVersionCheckLessThan(@"com.adobe.InCopy", -1, 5.1)
        || WKAppVersionCheckLessThan(@"com.adobe.InDesign", -1, 5.1)
        || WKAppVersionCheckLessThan(@"com.adobe.Soundbooth", -1, 2);

    return needsQuirk;
}

- (BOOL)_needsLinkElementTextCSSQuirk
{
    static BOOL needsQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_LINK_ELEMENT_TEXT_CSS_QUIRK)
        && WKAppVersionCheckLessThan(@"com.e-frontier.shade10", -1, 10.6);
    return needsQuirk;
}

- (BOOL)_needsIsLoadingInAPISenseQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.apple.iAdProducer", -1, 2.1);
    
    return needsQuirk;
}

- (BOOL)_needsKeyboardEventDisambiguationQuirks
{
    static BOOL needsQuirks = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_IE_COMPATIBLE_KEYBOARD_EVENT_DISPATCH) && !applicationIsSafari();
    return needsQuirks;
}

- (BOOL)_needsFrameLoadDelegateRetainQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.equinux.iSale5", -1, 5.6);    
    return needsQuirk;
}

static bool needsSelfRetainWhileLoadingQuirk()
{
    static bool needsQuirk = applicationIsAperture();
    return needsQuirk;
}
#endif // !PLATFORM(IOS)

- (BOOL)_needsPreHTML5ParserQuirks
{    
#if !PLATFORM(IOS)
    // AOL Instant Messenger and Microsoft My Day contain markup incompatible
    // with the new HTML5 parser. If these applications were linked against a
    // version of WebKit prior to the introduction of the HTML5 parser, enable
    // parser quirks to maintain compatibility. For details, see 
    // <https://bugs.webkit.org/show_bug.cgi?id=46134> and
    // <https://bugs.webkit.org/show_bug.cgi?id=46334>.
    static bool isApplicationNeedingParserQuirks = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER)
        && (applicationIsAOLInstantMessenger() || applicationIsMicrosoftMyDay());
    
    // Mail.app must continue to display HTML email that contains quirky markup.
    static bool isAppleMail = applicationIsAppleMail();

    return isApplicationNeedingParserQuirks
        || isAppleMail
#if ENABLE(DASHBOARD_SUPPORT)
        // Pre-HTML5 parser quirks are required to remain compatible with many
        // Dashboard widgets. See <rdar://problem/8175982>.
        || (_private->page && _private->page->settings().usesDashboardBackwardCompatibilityMode())
#endif
        || [[self preferences] usePreHTML5ParserQuirks];
#else
    static bool isApplicationNeedingParserQuirks = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER) && applicationIsDaijisenDictionary();
    return isApplicationNeedingParserQuirks || [[self preferences] usePreHTML5ParserQuirks];
#endif
}

- (void)_preferencesChangedNotification:(NSNotification *)notification
{
#if PLATFORM(IOS)
    // For WebViews that load synchronously, preference changes need to be propagated
    // down to WebCore synchronously so that loads in that WebView have the correct
    // up-to-date settings.
    if (!WebThreadIsLocked())
        WebThreadLock();
    if ([[self mainFrame] _loadsSynchronously]) {
#endif

    WebPreferences *preferences = (WebPreferences *)[notification object];
    [self _preferencesChanged:preferences];

#if PLATFORM(IOS)
    } else {
        WebThreadRun(^{
            // It is possible that the prefs object has already changed before the invocation could be called
            // on the web thread. This is not possible on TOT which is why they have a simple ASSERT.
            WebPreferences *preferences = (WebPreferences *)[notification object];
            if (preferences != [self preferences])
                return;
            [self _preferencesChanged:preferences];
        });
    }
#endif
}

- (void)_preferencesChanged:(WebPreferences *)preferences
{    
    ASSERT(preferences == [self preferences]);
    if (!_private->userAgentOverridden)
        _private->userAgent = String();

    // Cache this value so we don't have to read NSUserDefaults on each page load
    _private->useSiteSpecificSpoofing = [preferences _useSiteSpecificSpoofing];

    // Update corresponding WebCore Settings object.
    if (!_private->page)
        return;

#if PLATFORM(IOS)
    // iOS waits to call [WebDatabaseManager sharedWebDatabaseManager] until
    // didOneTimeInitialization is true so that we don't initialize databases
    // until the first WebView has been created. This is because database
    // initialization current requires disk access to populate the origins
    // quota map and we want to do this lazily by waiting until WebKit is
    // used to display web content in a WebView. The possible cases are:
    //   - on one time initialize (the first WebView creation) if databases are enabled (default)
    //   - when databases are dynamically enabled later, and they were disabled at startup (this case)
    if (didOneTimeInitialization) {
        if ([preferences databasesEnabled])
            [WebDatabaseManager sharedWebDatabaseManager];
        
        if ([preferences storageTrackerEnabled])
            WebKitInitializeStorageIfNecessary();
    }
#endif
    
    Settings& settings = _private->page->settings();

    settings.setCursiveFontFamily([preferences cursiveFontFamily]);
    settings.setDefaultFixedFontSize([preferences defaultFixedFontSize]);
    settings.setDefaultFontSize([preferences defaultFontSize]);
    settings.setDefaultTextEncodingName([preferences defaultTextEncodingName]);
    settings.setUsesEncodingDetector([preferences usesEncodingDetector]);
    settings.setFantasyFontFamily([preferences fantasyFontFamily]);
    settings.setFixedFontFamily([preferences fixedFontFamily]);
    settings.setForceFTPDirectoryListings([preferences _forceFTPDirectoryListings]);
    settings.setFTPDirectoryTemplatePath([preferences _ftpDirectoryTemplatePath]);
    settings.setLocalStorageDatabasePath([preferences _localStorageDatabasePath]);
    settings.setJavaEnabled([preferences isJavaEnabled]);
    settings.setScriptEnabled([preferences isJavaScriptEnabled]);
    settings.setWebSecurityEnabled([preferences isWebSecurityEnabled]);
    settings.setAllowUniversalAccessFromFileURLs([preferences allowUniversalAccessFromFileURLs]);
    settings.setAllowFileAccessFromFileURLs([preferences allowFileAccessFromFileURLs]);
    settings.setJavaScriptCanOpenWindowsAutomatically([preferences javaScriptCanOpenWindowsAutomatically]);
    settings.setMinimumFontSize([preferences minimumFontSize]);
    settings.setMinimumLogicalFontSize([preferences minimumLogicalFontSize]);
    settings.setPictographFontFamily([preferences pictographFontFamily]);
    settings.setPluginsEnabled([preferences arePlugInsEnabled]);
    settings.setLocalStorageEnabled([preferences localStorageEnabled]);

    _private->page->enableLegacyPrivateBrowsing([preferences privateBrowsingEnabled]);
    settings.setSansSerifFontFamily([preferences sansSerifFontFamily]);
    settings.setSerifFontFamily([preferences serifFontFamily]);
    settings.setStandardFontFamily([preferences standardFontFamily]);
    settings.setLoadsImagesAutomatically([preferences loadsImagesAutomatically]);
    settings.setLoadsSiteIconsIgnoringImageLoadingSetting([preferences loadsSiteIconsIgnoringImageLoadingPreference]);
#if PLATFORM(IOS)
    settings.setShouldPrintBackgrounds(true);
#else
    settings.setShouldPrintBackgrounds([preferences shouldPrintBackgrounds]);
#endif
    settings.setShrinksStandaloneImagesToFit([preferences shrinksStandaloneImagesToFit]);
    settings.setEditableLinkBehavior(core([preferences editableLinkBehavior]));
    settings.setTextDirectionSubmenuInclusionBehavior(core([preferences textDirectionSubmenuInclusionBehavior]));
    settings.setDOMPasteAllowed([preferences isDOMPasteAllowed]);
    settings.setUsesPageCache([self usesPageCache]);
    settings.setPageCacheSupportsPlugins([preferences pageCacheSupportsPlugins]);
    settings.setBackForwardCacheExpirationInterval([preferences _backForwardCacheExpirationInterval]);

    settings.setDeveloperExtrasEnabled([preferences developerExtrasEnabled]);
    settings.setJavaScriptRuntimeFlags(JSC::RuntimeFlags([preferences javaScriptRuntimeFlags]));
    settings.setAuthorAndUserStylesEnabled([preferences authorAndUserStylesEnabled]);

    settings.setNeedsSiteSpecificQuirks(_private->useSiteSpecificSpoofing);
    settings.setDOMTimersThrottlingEnabled([preferences domTimersThrottlingEnabled]);
    settings.setWebArchiveDebugModeEnabled([preferences webArchiveDebugModeEnabled]);
    settings.setLocalFileContentSniffingEnabled([preferences localFileContentSniffingEnabled]);
    settings.setOfflineWebApplicationCacheEnabled([preferences offlineWebApplicationCacheEnabled]);
    settings.setJavaScriptCanAccessClipboard([preferences javaScriptCanAccessClipboard]);
    settings.setXSSAuditorEnabled([preferences isXSSAuditorEnabled]);
    settings.setDNSPrefetchingEnabled([preferences isDNSPrefetchingEnabled]);
    
    // FIXME: Enabling accelerated compositing when WebGL is enabled causes tests to fail on Leopard which expect HW compositing to be disabled.
    // Until we fix that, I will comment out the test (CFM)
    settings.setAcceleratedCompositingEnabled([preferences acceleratedCompositingEnabled]);
    settings.setAcceleratedDrawingEnabled([preferences acceleratedDrawingEnabled]);
    settings.setCanvasUsesAcceleratedDrawing([preferences canvasUsesAcceleratedDrawing]);    
    settings.setShowDebugBorders([preferences showDebugBorders]);
    settings.setSimpleLineLayoutDebugBordersEnabled([preferences simpleLineLayoutDebugBordersEnabled]);
    settings.setShowRepaintCounter([preferences showRepaintCounter]);
    settings.setWebGLEnabled([preferences webGLEnabled]);
    settings.setSubpixelCSSOMElementMetricsEnabled([preferences subpixelCSSOMElementMetricsEnabled]);

    settings.setForceSoftwareWebGLRendering([preferences forceSoftwareWebGLRendering]);
    settings.setAccelerated2dCanvasEnabled([preferences accelerated2dCanvasEnabled]);
    settings.setLoadDeferringEnabled(shouldEnableLoadDeferring());
    settings.setWindowFocusRestricted(shouldRestrictWindowFocus());
    settings.setFrameFlatteningEnabled([preferences isFrameFlatteningEnabled]);
    settings.setSpatialNavigationEnabled([preferences isSpatialNavigationEnabled]);
    settings.setPaginateDuringLayoutEnabled([preferences paginateDuringLayoutEnabled]);
    RuntimeEnabledFeatures::sharedFeatures().setCSSRegionsEnabled([preferences cssRegionsEnabled]);
    RuntimeEnabledFeatures::sharedFeatures().setCSSCompositingEnabled([preferences cssCompositingEnabled]);

    settings.setAsynchronousSpellCheckingEnabled([preferences asynchronousSpellCheckingEnabled]);
    settings.setHyperlinkAuditingEnabled([preferences hyperlinkAuditingEnabled]);
    settings.setUsePreHTML5ParserQuirks([self _needsPreHTML5ParserQuirks]);
    settings.setInteractiveFormValidationEnabled([self interactiveFormValidationEnabled]);
    settings.setValidationMessageTimerMagnification([self validationMessageTimerMagnification]);

    settings.setMediaPlaybackRequiresUserGesture([preferences mediaPlaybackRequiresUserGesture]);
    settings.setMediaPlaybackAllowsInline([preferences mediaPlaybackAllowsInline]);
    settings.setAllowsAlternateFullscreen([preferences allowsAlternateFullscreen]);
    settings.setSuppressesIncrementalRendering([preferences suppressesIncrementalRendering]);
    settings.setBackspaceKeyNavigationEnabled([preferences backspaceKeyNavigationEnabled]);
    settings.setWantsBalancedSetDefersLoadingBehavior([preferences wantsBalancedSetDefersLoadingBehavior]);
    settings.setMockScrollbarsEnabled([preferences mockScrollbarsEnabled]);

    settings.setShouldRespectImageOrientation([preferences shouldRespectImageOrientation]);

    settings.setRequestAnimationFrameEnabled([preferences requestAnimationFrameEnabled]);
    settings.setDiagnosticLoggingEnabled([preferences diagnosticLoggingEnabled]);
    settings.setLowPowerVideoAudioBufferSizeEnabled([preferences lowPowerVideoAudioBufferSizeEnabled]);

    settings.setUseLegacyTextAlignPositionedElementBehavior([preferences useLegacyTextAlignPositionedElementBehavior]);

    settings.setShouldConvertPositionStyleOnCopy([preferences shouldConvertPositionStyleOnCopy]);
    settings.setEnableInheritURIQueryComponent([preferences isInheritURIQueryComponentEnabled]);

    switch ([preferences storageBlockingPolicy]) {
    case WebAllowAllStorage:
        settings.setStorageBlockingPolicy(SecurityOrigin::AllowAllStorage);
        break;
    case WebBlockThirdPartyStorage:
        settings.setStorageBlockingPolicy(SecurityOrigin::BlockThirdPartyStorage);
        break;
    case WebBlockAllStorage:
        settings.setStorageBlockingPolicy(SecurityOrigin::BlockAllStorage);
        break;
    }

    settings.setPlugInSnapshottingEnabled([preferences plugInSnapshottingEnabled]);

    settings.setFixedPositionCreatesStackingContext(true);
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    settings.setAcceleratedCompositingForFixedPositionEnabled(true);
#endif

#if ENABLE(RUBBER_BANDING)
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=136131
    settings.setRubberBandingForSubScrollableRegionsEnabled(false);
#endif

#if PLATFORM(IOS)
    settings.setStandalone([preferences _standalone]);
    settings.setTelephoneNumberParsingEnabled([preferences _telephoneNumberParsingEnabled]);
    settings.setAllowMultiElementImplicitSubmission([preferences _allowMultiElementImplicitFormSubmission]);
    settings.setLayoutInterval(std::chrono::milliseconds([preferences _layoutInterval]));
    settings.setMaxParseDuration([preferences _maxParseDuration]);
    settings.setAlwaysUseAcceleratedOverflowScroll([preferences _alwaysUseAcceleratedOverflowScroll]);
    settings.setMediaPlaybackAllowsAirPlay([preferences mediaPlaybackAllowsAirPlay]);
    settings.setAudioSessionCategoryOverride([preferences audioSessionCategoryOverride]);
    settings.setNetworkDataUsageTrackingEnabled([preferences networkDataUsageTrackingEnabled]);
    settings.setNetworkInterfaceName([preferences networkInterfaceName]);
    settings.setAVKitEnabled([preferences avKitEnabled]);
    settings.setShouldTransformsAffectOverflow(shouldTransformsAffectOverflow());
    settings.setShouldDispatchJavaScriptWindowOnErrorEvents(shouldDispatchJavaScriptWindowOnErrorEvents());

    settings.setPasswordEchoEnabled([preferences _allowPasswordEcho]);
    settings.setPasswordEchoDurationInSeconds([preferences _passwordEchoDuration]);

    ASSERT_WITH_MESSAGE(settings.pageCacheSupportsPlugins(), "PageCacheSupportsPlugins should be enabled on iOS.");
    settings.setDelegatesPageScaling(true);

#if ENABLE(IOS_TEXT_AUTOSIZING)
    settings.setMinimumZoomFontSize([preferences _minimumZoomFontSize]);
#endif
#endif // PLATFORM(IOS)

#if PLATFORM(MAC)
    if ([preferences userStyleSheetEnabled]) {
        NSString* location = [[preferences userStyleSheetLocation] _web_originalDataAsString];
        if ([location isEqualToString:@"apple-dashboard://stylesheet"])
            location = @"file:///System/Library/PrivateFrameworks/DashboardClient.framework/Resources/widget.css";
        settings.setUserStyleSheetLocation([NSURL URLWithString:(location ? location : @"")]);
    } else
        settings.setUserStyleSheetLocation([NSURL URLWithString:@""]);

    settings.setNeedsAdobeFrameReloadingQuirk([self _needsAdobeFrameReloadingQuirk]);
    settings.setTreatsAnyTextCSSLinkAsStylesheet([self _needsLinkElementTextCSSQuirk]);
    settings.setNeedsKeyboardEventDisambiguationQuirks([self _needsKeyboardEventDisambiguationQuirks]);
    settings.setEnforceCSSMIMETypeInNoQuirksMode(!WKAppVersionCheckLessThan(@"com.apple.iWeb", -1, 2.1));
    settings.setNeedsIsLoadingInAPISenseQuirk([self _needsIsLoadingInAPISenseQuirk]);
    settings.setTextAreasAreResizable([preferences textAreasAreResizable]);
    settings.setExperimentalNotificationsEnabled([preferences experimentalNotificationsEnabled]);
    settings.setShowsURLsInToolTips([preferences showsURLsInToolTips]);
    settings.setShowsToolTipOverTruncatedText([preferences showsToolTipOverTruncatedText]);
    settings.setQTKitEnabled([preferences isQTKitEnabled]);
#endif // PLATFORM(MAC)

    DatabaseManager::singleton().setIsAvailable([preferences databasesEnabled]);

#if ENABLE(MEDIA_SOURCE)
    settings.setMediaSourceEnabled([preferences mediaSourceEnabled]);
#endif

#if ENABLE(SERVICE_CONTROLS)
    settings.setImageControlsEnabled([preferences imageControlsEnabled]);
    settings.setServiceControlsEnabled([preferences serviceControlsEnabled]);
#endif

#if ENABLE(VIDEO_TRACK)
    settings.setShouldDisplaySubtitles([preferences shouldDisplaySubtitles]);
    settings.setShouldDisplayCaptions([preferences shouldDisplayCaptions]);
    settings.setShouldDisplayTextDescriptions([preferences shouldDisplayTextDescriptions]);
#endif

#if USE(AVFOUNDATION)
    settings.setAVFoundationEnabled([preferences isAVFoundationEnabled]);
#endif

#if ENABLE(WEB_AUDIO)
    settings.setWebAudioEnabled([preferences webAudioEnabled]);
#endif

#if ENABLE(FULLSCREEN_API)
    settings.setFullScreenEnabled([preferences fullScreenEnabled]);
#endif

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    settings.setHiddenPageDOMTimerThrottlingEnabled([preferences hiddenPageDOMTimerThrottlingEnabled]);
#endif

    settings.setHiddenPageCSSAnimationSuspensionEnabled([preferences hiddenPageCSSAnimationSuspensionEnabled]);

#if ENABLE(GAMEPAD)
    RuntimeEnabledFeatures::sharedFeatures().setGamepadsEnabled([preferences gamepadsEnabled]);
#endif

    NSTimeInterval timeout = [preferences incrementalRenderingSuppressionTimeoutInSeconds];
    if (timeout > 0)
        settings.setIncrementalRenderingSuppressionTimeoutInSeconds(timeout);

    // Application Cache Preferences are stored on the global cache storage manager, not in Settings.
    [WebApplicationCache setDefaultOriginQuota:[preferences applicationCacheDefaultOriginQuota]];

    BOOL zoomsTextOnly = [preferences zoomsTextOnly];
    if (_private->zoomsTextOnly != zoomsTextOnly)
        [self _setZoomMultiplier:_private->zoomMultiplier isTextOnly:zoomsTextOnly];

#if PLATFORM(IOS)
    [[self window] setTileBordersVisible:[preferences showDebugBorders]];
    [[self window] setTilePaintCountsVisible:[preferences showRepaintCounter]];
    [[self window] setAcceleratedDrawingEnabled:[preferences acceleratedDrawingEnabled]];
    [WAKView _setInterpolationQuality:[preferences _interpolationQuality]];
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    settings.setMediaKeysStorageDirectory([preferences mediaKeysStorageDirectory]);
#endif
}

static inline IMP getMethod(id o, SEL s)
{
    return [o respondsToSelector:s] ? [o methodForSelector:s] : 0;
}

#if PLATFORM(IOS)
- (id)_UIKitDelegateForwarder
{
    if (_private->closing)
        return nil;
        
    if (!_private->UIKitDelegateForwarder)
        _private->UIKitDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->UIKitDelegate defaultTarget:[WebDefaultUIKitDelegate sharedUIKitDelegate]];
    return _private->UIKitDelegateForwarder;
}
#endif

- (void)_cacheResourceLoadDelegateImplementations
{
    WebResourceDelegateImplementationCache *cache = &_private->resourceLoadDelegateImplementations;
    id delegate = _private->resourceProgressDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebResourceDelegateImplementationCache));
        return;
    }

    cache->didCancelAuthenticationChallengeFunc = getMethod(delegate, @selector(webView:resource:didCancelAuthenticationChallenge:fromDataSource:));
    cache->didFailLoadingWithErrorFromDataSourceFunc = getMethod(delegate, @selector(webView:resource:didFailLoadingWithError:fromDataSource:));
    cache->didFinishLoadingFromDataSourceFunc = getMethod(delegate, @selector(webView:resource:didFinishLoadingFromDataSource:));
    cache->didLoadResourceFromMemoryCacheFunc = getMethod(delegate, @selector(webView:didLoadResourceFromMemoryCache:response:length:fromDataSource:));
    cache->didReceiveAuthenticationChallengeFunc = getMethod(delegate, @selector(webView:resource:didReceiveAuthenticationChallenge:fromDataSource:));
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    cache->canAuthenticateAgainstProtectionSpaceFunc = getMethod(delegate, @selector(webView:resource:canAuthenticateAgainstProtectionSpace:forDataSource:));
#endif

#if PLATFORM(IOS)
    cache->connectionPropertiesFunc = getMethod(delegate, @selector(webView:connectionPropertiesForResource:dataSource:));
    cache->webThreadDidFinishLoadingFromDataSourceFunc = getMethod(delegate, @selector(webThreadWebView:resource:didFinishLoadingFromDataSource:));
    cache->webThreadDidFailLoadingWithErrorFromDataSourceFunc = getMethod(delegate, @selector(webThreadWebView:resource:didFailLoadingWithError:fromDataSource:));
    cache->webThreadIdentifierForRequestFunc = getMethod(delegate, @selector(webThreadWebView:identifierForInitialRequest:fromDataSource:));
    cache->webThreadDidLoadResourceFromMemoryCacheFunc = getMethod(delegate, @selector(webThreadWebView:didLoadResourceFromMemoryCache:response:length:fromDataSource:));
    cache->webThreadWillSendRequestFunc = getMethod(delegate, @selector(webThreadWebView:resource:willSendRequest:redirectResponse:fromDataSource:));
    cache->webThreadDidReceiveResponseFunc = getMethod(delegate, @selector(webThreadWebView:resource:didReceiveResponse:fromDataSource:));
    cache->webThreadDidReceiveContentLengthFunc = getMethod(delegate, @selector(webThreadWebView:resource:didReceiveContentLength:fromDataSource:));
    cache->webThreadWillCacheResponseFunc = getMethod(delegate, @selector(webThreadWebView:resource:willCacheResponse:fromDataSource:));
#endif

    cache->didReceiveContentLengthFunc = getMethod(delegate, @selector(webView:resource:didReceiveContentLength:fromDataSource:));
    cache->didReceiveResponseFunc = getMethod(delegate, @selector(webView:resource:didReceiveResponse:fromDataSource:));
    cache->identifierForRequestFunc = getMethod(delegate, @selector(webView:identifierForInitialRequest:fromDataSource:));
    cache->plugInFailedWithErrorFunc = getMethod(delegate, @selector(webView:plugInFailedWithError:dataSource:));
    cache->willCacheResponseFunc = getMethod(delegate, @selector(webView:resource:willCacheResponse:fromDataSource:));
    cache->willSendRequestFunc = getMethod(delegate, @selector(webView:resource:willSendRequest:redirectResponse:fromDataSource:));
    cache->shouldUseCredentialStorageFunc = getMethod(delegate, @selector(webView:resource:shouldUseCredentialStorageForDataSource:));
    cache->shouldPaintBrokenImageForURLFunc = getMethod(delegate, @selector(webView:shouldPaintBrokenImageForURL:));
}

- (void)_cacheFrameLoadDelegateImplementations
{
    WebFrameLoadDelegateImplementationCache *cache = &_private->frameLoadDelegateImplementations;
    id delegate = _private->frameLoadDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebFrameLoadDelegateImplementationCache));
        return;
    }

    cache->didCancelClientRedirectForFrameFunc = getMethod(delegate, @selector(webView:didCancelClientRedirectForFrame:));
    cache->didChangeLocationWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didChangeLocationWithinPageForFrame:));
    cache->didPushStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didPushStateWithinPageForFrame:));
    cache->didReplaceStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didReplaceStateWithinPageForFrame:));
    cache->didPopStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didPopStateWithinPageForFrame:));
#if JSC_OBJC_API_ENABLED
    cache->didCreateJavaScriptContextForFrameFunc = getMethod(delegate, @selector(webView:didCreateJavaScriptContext:forFrame:));
#endif
    cache->didClearWindowObjectForFrameFunc = getMethod(delegate, @selector(webView:didClearWindowObject:forFrame:));
    cache->didClearWindowObjectForFrameInScriptWorldFunc = getMethod(delegate, @selector(webView:didClearWindowObjectForFrame:inScriptWorld:));
    cache->didClearInspectorWindowObjectForFrameFunc = getMethod(delegate, @selector(webView:didClearInspectorWindowObject:forFrame:));
    cache->didCommitLoadForFrameFunc = getMethod(delegate, @selector(webView:didCommitLoadForFrame:));
    cache->didFailLoadWithErrorForFrameFunc = getMethod(delegate, @selector(webView:didFailLoadWithError:forFrame:));
    cache->didFailProvisionalLoadWithErrorForFrameFunc = getMethod(delegate, @selector(webView:didFailProvisionalLoadWithError:forFrame:));
    cache->didFinishDocumentLoadForFrameFunc = getMethod(delegate, @selector(webView:didFinishDocumentLoadForFrame:));
    cache->didFinishLoadForFrameFunc = getMethod(delegate, @selector(webView:didFinishLoadForFrame:));
    cache->didFirstLayoutInFrameFunc = getMethod(delegate, @selector(webView:didFirstLayoutInFrame:));
    cache->didFirstVisuallyNonEmptyLayoutInFrameFunc = getMethod(delegate, @selector(webView:didFirstVisuallyNonEmptyLayoutInFrame:));
    cache->didLayoutFunc = getMethod(delegate, @selector(webView:didLayout:));
    cache->didHandleOnloadEventsForFrameFunc = getMethod(delegate, @selector(webView:didHandleOnloadEventsForFrame:));
#if ENABLE(ICONDATABASE)
    cache->didReceiveIconForFrameFunc = getMethod(delegate, @selector(webView:didReceiveIcon:forFrame:));
#endif
    cache->didReceiveServerRedirectForProvisionalLoadForFrameFunc = getMethod(delegate, @selector(webView:didReceiveServerRedirectForProvisionalLoadForFrame:));
    cache->didReceiveTitleForFrameFunc = getMethod(delegate, @selector(webView:didReceiveTitle:forFrame:));
    cache->didStartProvisionalLoadForFrameFunc = getMethod(delegate, @selector(webView:didStartProvisionalLoadForFrame:));
    cache->willCloseFrameFunc = getMethod(delegate, @selector(webView:willCloseFrame:));
    cache->willPerformClientRedirectToURLDelayFireDateForFrameFunc = getMethod(delegate, @selector(webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:));
    cache->windowScriptObjectAvailableFunc = getMethod(delegate, @selector(webView:windowScriptObjectAvailable:));
    cache->didDisplayInsecureContentFunc = getMethod(delegate, @selector(webViewDidDisplayInsecureContent:));
    cache->didRunInsecureContentFunc = getMethod(delegate, @selector(webView:didRunInsecureContent:));
    cache->didDetectXSSFunc = getMethod(delegate, @selector(webView:didDetectXSS:));
    cache->didRemoveFrameFromHierarchyFunc = getMethod(delegate, @selector(webView:didRemoveFrameFromHierarchy:));
#if PLATFORM(IOS)
    cache->webThreadDidLayoutFunc = getMethod(delegate, @selector(webThreadWebView:didLayout:));
#endif

    // It would be nice to get rid of this code and transition all clients to using didLayout instead of
    // didFirstLayoutInFrame and didFirstVisuallyNonEmptyLayoutInFrame. In the meantime, this is required
    // for backwards compatibility.
    Page* page = core(self);
    if (page) {
        unsigned milestones = DidFirstLayout;
#if PLATFORM(IOS)
        milestones |= DidFirstVisuallyNonEmptyLayout;
#else
        if (cache->didFirstVisuallyNonEmptyLayoutInFrameFunc)
            milestones |= DidFirstVisuallyNonEmptyLayout;
#endif
        page->addLayoutMilestones(static_cast<LayoutMilestones>(milestones));
    }
}

- (void)_cacheScriptDebugDelegateImplementations
{
    WebScriptDebugDelegateImplementationCache *cache = &_private->scriptDebugDelegateImplementations;
    id delegate = _private->scriptDebugDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebScriptDebugDelegateImplementationCache));
        return;
    }

    cache->didParseSourceFunc = getMethod(delegate, @selector(webView:didParseSource:baseLineNumber:fromURL:sourceId:forWebFrame:));
    if (cache->didParseSourceFunc)
        cache->didParseSourceExpectsBaseLineNumber = YES;
    else {
        cache->didParseSourceExpectsBaseLineNumber = NO;
        cache->didParseSourceFunc = getMethod(delegate, @selector(webView:didParseSource:fromURL:sourceId:forWebFrame:));
    }

    cache->failedToParseSourceFunc = getMethod(delegate, @selector(webView:failedToParseSource:baseLineNumber:fromURL:withError:forWebFrame:));

    cache->exceptionWasRaisedFunc = getMethod(delegate, @selector(webView:exceptionWasRaised:hasHandler:sourceId:line:forWebFrame:));
    if (cache->exceptionWasRaisedFunc)
        cache->exceptionWasRaisedExpectsHasHandlerFlag = YES;
    else {
        cache->exceptionWasRaisedExpectsHasHandlerFlag = NO;
        cache->exceptionWasRaisedFunc = getMethod(delegate, @selector(webView:exceptionWasRaised:sourceId:line:forWebFrame:));
    }
}

- (void)_cacheHistoryDelegateImplementations
{
    WebHistoryDelegateImplementationCache *cache = &_private->historyDelegateImplementations;
    id delegate = _private->historyDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebHistoryDelegateImplementationCache));
        return;
    }

    cache->navigatedFunc = getMethod(delegate, @selector(webView:didNavigateWithNavigationData:inFrame:));
    cache->clientRedirectFunc = getMethod(delegate, @selector(webView:didPerformClientRedirectFromURL:toURL:inFrame:));
    cache->serverRedirectFunc = getMethod(delegate, @selector(webView:didPerformServerRedirectFromURL:toURL:inFrame:));
    cache->deprecatedSetTitleFunc = getMethod(delegate, @selector(webView:updateHistoryTitle:forURL:));
    cache->setTitleFunc = getMethod(delegate, @selector(webView:updateHistoryTitle:forURL:inFrame:));
    cache->populateVisitedLinksFunc = getMethod(delegate, @selector(populateVisitedLinksForWebView:));
}

- (id)_policyDelegateForwarder
{
#if PLATFORM(IOS)
    if (_private->closing)
        return nil;
#endif
    if (!_private->policyDelegateForwarder)
        _private->policyDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->policyDelegate defaultTarget:[WebDefaultPolicyDelegate sharedPolicyDelegate]];
    return _private->policyDelegateForwarder;
}

- (id)_UIDelegateForwarder
{
#if PLATFORM(IOS)
    if (_private->closing)
        return nil;
#endif
    if (!_private->UIDelegateForwarder)
        _private->UIDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->UIDelegate defaultTarget:[WebDefaultUIDelegate sharedUIDelegate]];
    return _private->UIDelegateForwarder;
}

#if PLATFORM(IOS)
- (id)_UIDelegateForSelector:(SEL)selector
{
    id delegate = self->_private->UIDelegate;
    if ([delegate respondsToSelector:selector])
        return delegate;

    delegate = [WebDefaultUIDelegate sharedUIDelegate];
    if ([delegate respondsToSelector:selector])
        return delegate;

    return nil;
}
#endif

- (id)_editingDelegateForwarder
{
#if PLATFORM(IOS)
    if (_private->closing)
        return nil;
#endif    
    // This can be called during window deallocation by QTMovieView in the QuickTime Cocoa Plug-in.
    // Not sure if that is a bug or not.
    if (!_private)
        return nil;

    if (!_private->editingDelegateForwarder)
        _private->editingDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->editingDelegate defaultTarget:[WebDefaultEditingDelegate sharedEditingDelegate]];
    return _private->editingDelegateForwarder;
}

- (void)_closeWindow
{
    [[self _UIDelegateForwarder] webViewClose:self];
}

+ (void)_unregisterViewClassAndRepresentationClassForMIMEType:(NSString *)MIMEType
{
    [[WebFrameView _viewTypesAllowImageTypeOmission:NO] removeObjectForKey:MIMEType];
    [[WebDataSource _repTypesAllowImageTypeOmission:NO] removeObjectForKey:MIMEType];
    
    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    MIMETypeRegistry::getSupportedNonImageMIMETypes().remove(MIMEType);
}

+ (void)_registerViewClass:(Class)viewClass representationClass:(Class)representationClass forURLScheme:(NSString *)URLScheme
{
    NSString *MIMEType = [self _generatedMIMETypeForURLScheme:URLScheme];
    [self registerViewClass:viewClass representationClass:representationClass forMIMEType:MIMEType];

    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    if ([viewClass class] == [WebHTMLView class])
        MIMETypeRegistry::getSupportedNonImageMIMETypes().add(MIMEType);
    
    // This is used to make _representationExistsForURLScheme faster.
    // Without this set, we'd have to create the MIME type each time.
    if (schemesWithRepresentationsSet == nil) {
        schemesWithRepresentationsSet = [[NSMutableSet alloc] init];
    }
    [schemesWithRepresentationsSet addObject:[[[URLScheme lowercaseString] copy] autorelease]];
}

+ (NSString *)_generatedMIMETypeForURLScheme:(NSString *)URLScheme
{
    return [@"x-apple-web-kit/" stringByAppendingString:[URLScheme lowercaseString]];
}

+ (BOOL)_representationExistsForURLScheme:(NSString *)URLScheme
{
    return [schemesWithRepresentationsSet containsObject:[URLScheme lowercaseString]];
}

+ (BOOL)_canHandleRequest:(NSURLRequest *)request forMainFrame:(BOOL)forMainFrame
{
    // FIXME: If <rdar://problem/5217309> gets fixed, this check can be removed.
    if (!request)
        return NO;

    if ([NSURLConnection canHandleRequest:request])
        return YES;

    NSString *scheme = [[request URL] scheme];

    // Representations for URL schemes work at the top level.
    if (forMainFrame && [self _representationExistsForURLScheme:scheme])
        return YES;

    if ([scheme _webkit_isCaseInsensitiveEqualToString:@"applewebdata"])
        return YES;

    if ([scheme _webkit_isCaseInsensitiveEqualToString:@"blob"])
        return YES;

    return NO;
}

+ (BOOL)_canHandleRequest:(NSURLRequest *)request
{
    return [self _canHandleRequest:request forMainFrame:YES];
}

+ (NSString *)_decodeData:(NSData *)data
{
    HTMLNames::init(); // this method is used for importing bookmarks at startup, so HTMLNames are likely to be uninitialized yet
    return TextResourceDecoder::create("text/html")->decodeAndFlush(static_cast<const char*>([data bytes]), [data length]); // bookmark files are HTML
}

- (void)_pushPerformingProgrammaticFocus
{
    _private->programmaticFocusCount++;
}

- (void)_popPerformingProgrammaticFocus
{
    _private->programmaticFocusCount--;
}

- (BOOL)_isPerformingProgrammaticFocus
{
    return _private->programmaticFocusCount != 0;
}

#if !PLATFORM(IOS)
- (void)_didChangeValueForKey: (NSString *)key
{
    LOG (Bindings, "calling didChangeValueForKey: %@", key);
    [self didChangeValueForKey: key];
}

- (void)_willChangeValueForKey: (NSString *)key
{
    LOG (Bindings, "calling willChangeValueForKey: %@", key);
    [self willChangeValueForKey: key];
}

+ (BOOL)automaticallyNotifiesObserversForKey:(NSString *)key {
    static NSSet *manualNotifyKeys = nil;
    if (!manualNotifyKeys)
        manualNotifyKeys = [[NSSet alloc] initWithObjects:_WebMainFrameURLKey, _WebIsLoadingKey, _WebEstimatedProgressKey,
            _WebCanGoBackKey, _WebCanGoForwardKey, _WebMainFrameTitleKey, _WebMainFrameIconKey, _WebMainFrameDocumentKey,
            nil];
    if ([manualNotifyKeys containsObject:key])
        return NO;
    return YES;
}

- (NSArray *)_declaredKeys {
    static NSArray *declaredKeys = nil;
    if (!declaredKeys)
        declaredKeys = [[NSArray alloc] initWithObjects:_WebMainFrameURLKey, _WebIsLoadingKey, _WebEstimatedProgressKey,
            _WebCanGoBackKey, _WebCanGoForwardKey, _WebMainFrameTitleKey, _WebMainFrameIconKey, _WebMainFrameDocumentKey, nil];
    return declaredKeys;
}

- (void)setObservationInfo:(void *)info
{
    _private->observationInfo = info;
}

- (void *)observationInfo
{
    return _private->observationInfo;
}

- (void)_willChangeBackForwardKeys
{
    [self _willChangeValueForKey: _WebCanGoBackKey];
    [self _willChangeValueForKey: _WebCanGoForwardKey];
}

- (void)_didChangeBackForwardKeys
{
    [self _didChangeValueForKey: _WebCanGoBackKey];
    [self _didChangeValueForKey: _WebCanGoForwardKey];
}

- (void)_didStartProvisionalLoadForFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self retain];

    [self _willChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];

        [self _willChangeValueForKey: _WebMainFrameURLKey];
    }

    [NSApp setWindowsNeedUpdate:YES];

#if ENABLE(FULLSCREEN_API)
    Document* document = core([frame DOMDocument]);
    if (Element* element = document ? document->webkitCurrentFullScreenElement() : 0) {
        SEL selector = @selector(webView:closeFullScreenWithListener:);
        if (_private->UIDelegate && [_private->UIDelegate respondsToSelector:selector]) {
            WebKitFullScreenListener *listener = [[WebKitFullScreenListener alloc] initWithElement:element];
            CallUIDelegate(self, selector, listener);
            [listener release];
        } else if (_private->newFullscreenController && [_private->newFullscreenController isFullScreen]) {
            [_private->newFullscreenController close];
        }
    }
#endif
}

- (void)_checkDidPerformFirstNavigation
{
    if (_private->_didPerformFirstNavigation)
        return;

    Page* page = _private->page;
    if (!page)
        return;

    auto& backForwardController = page->backForward();

    if (!backForwardController.backItem())
        return;

    _private->_didPerformFirstNavigation = YES;

    if (_private->preferences.automaticallyDetectsCacheModel && _private->preferences.cacheModel < WebCacheModelDocumentBrowser)
        _private->preferences.cacheModel = WebCacheModelDocumentBrowser;
}

- (void)_didCommitLoadForFrame:(WebFrame *)frame
{
    if (frame == [self mainFrame])
        [self _didChangeValueForKey: _WebMainFrameURLKey];

    [self _checkDidPerformFirstNavigation];

    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFinishLoadForFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];
        
    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];

    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];

    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
        
        [self _didChangeValueForKey: _WebMainFrameURLKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (NSCachedURLResponse *)_cachedResponseForURL:(NSURL *)URL
{
    RetainPtr<NSMutableURLRequest *> request = adoptNS([[NSMutableURLRequest alloc] initWithURL:URL]);
    [request _web_setHTTPUserAgent:[self userAgentForURL:URL]];
    NSCachedURLResponse *cachedResponse;

    if (!_private->page)
        return nil;

    if (CFURLStorageSessionRef storageSession = _private->page->mainFrame().loader().networkingContext()->storageSession().platformSession())
        cachedResponse = WKCachedResponseForRequest(storageSession, request.get());
    else
        cachedResponse = [[NSURLCache sharedURLCache] cachedResponseForRequest:request.get()];

    return cachedResponse;
}

- (void)_writeImageForElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    NSURL *linkURL = [element objectForKey:WebElementLinkURLKey];
    DOMElement *domElement = [element objectForKey:WebElementDOMNodeKey];
    [pasteboard _web_writeImage:(NSImage *)(domElement ? nil : [element objectForKey:WebElementImageKey])
                        element:domElement
                            URL:linkURL ? linkURL : (NSURL *)[element objectForKey:WebElementImageURLKey]
                          title:[element objectForKey:WebElementImageAltStringKey] 
                        archive:[[element objectForKey:WebElementDOMNodeKey] webArchive]
                          types:types
                         source:nil];
}

- (void)_writeLinkElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    [pasteboard _web_writeURL:[element objectForKey:WebElementLinkURLKey]
                     andTitle:[element objectForKey:WebElementLinkLabelKey]
                        types:types];
}

#if ENABLE(DRAG_SUPPORT)
- (void)_setInitiatedDrag:(BOOL)initiatedDrag
{
    if (!_private->page)
        return;
    _private->page->dragController().setDidInitiateDrag(initiatedDrag);
}
#endif

#else

- (void)_didCommitLoadForFrame:(WebFrame *)frame
{
    if (frame == [self mainFrame])
        _private->didDrawTiles = 0;
}

#endif // PLATFORM(IOS)

#if ENABLE(DASHBOARD_SUPPORT)

#define DASHBOARD_CONTROL_LABEL @"control"

- (void)_addControlRect:(NSRect)bounds clip:(NSRect)clip fromView:(NSView *)view toDashboardRegions:(NSMutableDictionary *)regions
{
    NSRect adjustedBounds = bounds;
    adjustedBounds.origin = [self convertPoint:bounds.origin fromView:view];
    adjustedBounds.origin.y = [self bounds].size.height - adjustedBounds.origin.y;
    adjustedBounds.size = bounds.size;

    NSRect adjustedClip;
    adjustedClip.origin = [self convertPoint:clip.origin fromView:view];
    adjustedClip.origin.y = [self bounds].size.height - adjustedClip.origin.y;
    adjustedClip.size = clip.size;

    WebDashboardRegion *region = [[WebDashboardRegion alloc] initWithRect:adjustedBounds 
        clip:adjustedClip type:WebDashboardRegionTypeScrollerRectangle];
    NSMutableArray *scrollerRegions = [regions objectForKey:DASHBOARD_CONTROL_LABEL];
    if (!scrollerRegions) {
        scrollerRegions = [[NSMutableArray alloc] init];
        [regions setObject:scrollerRegions forKey:DASHBOARD_CONTROL_LABEL];
        [scrollerRegions release];
    }
    [scrollerRegions addObject:region];
    [region release];
}

- (void)_addScrollerDashboardRegionsForFrameView:(FrameView*)frameView dashboardRegions:(NSMutableDictionary *)regions
{    
    NSView *documentView = [[kit(&frameView->frame()) frameView] documentView];

    for (const auto& widget: frameView->children()) {
        if (is<FrameView>(*widget)) {
            [self _addScrollerDashboardRegionsForFrameView:downcast<FrameView>(widget.get()) dashboardRegions:regions];
            continue;
        }

        if (!widget->isScrollbar())
            continue;

        // FIXME: This should really pass an appropriate clip, but our first try got it wrong, and
        // it's not common to need this to be correct in Dashboard widgets.
        NSRect bounds = widget->frameRect();
        [self _addControlRect:bounds clip:bounds fromView:documentView toDashboardRegions:regions];
    }
}

- (void)_addScrollerDashboardRegions:(NSMutableDictionary *)regions from:(NSArray *)views
{
    // Add scroller regions for NSScroller and WebCore scrollbars
    NSUInteger count = [views count];
    for (NSUInteger i = 0; i < count; i++) {
        NSView *view = [views objectAtIndex:i];
        
        if ([view isKindOfClass:[WebHTMLView class]]) {
            if (Frame* coreFrame = core([(WebHTMLView*)view _frame])) {
                if (FrameView* coreView = coreFrame->view())
                    [self _addScrollerDashboardRegionsForFrameView:coreView dashboardRegions:regions];
            }
        } else if ([view isKindOfClass:[NSScroller class]]) {
            // AppKit places absent scrollers at -100,-100
            if ([view frame].origin.y < 0)
                continue;
            [self _addControlRect:[view bounds] clip:[view visibleRect] fromView:view toDashboardRegions:regions];
        }
        [self _addScrollerDashboardRegions:regions from:[view subviews]];
    }
}

- (void)_addScrollerDashboardRegions:(NSMutableDictionary *)regions
{
    [self _addScrollerDashboardRegions:regions from:[self subviews]];
}

- (NSDictionary *)_dashboardRegions
{
    // Only return regions from main frame.
    Frame* mainFrame = [self _mainCoreFrame];
    if (!mainFrame)
        return nil;

    const Vector<AnnotatedRegionValue>& regions = mainFrame->document()->annotatedRegions();
    size_t size = regions.size();

    NSMutableDictionary *webRegions = [NSMutableDictionary dictionaryWithCapacity:size];
    for (size_t i = 0; i < size; i++) {
        const AnnotatedRegionValue& region = regions[i];

        if (region.type == StyleDashboardRegion::None)
            continue;

        NSString *label = region.label;
        WebDashboardRegionType type = WebDashboardRegionTypeNone;
        if (region.type == StyleDashboardRegion::Circle)
            type = WebDashboardRegionTypeCircle;
        else if (region.type == StyleDashboardRegion::Rectangle)
            type = WebDashboardRegionTypeRectangle;
        NSMutableArray *regionValues = [webRegions objectForKey:label];
        if (!regionValues) {
            regionValues = [[NSMutableArray alloc] initWithCapacity:1];
            [webRegions setObject:regionValues forKey:label];
            [regionValues release];
        }

        WebDashboardRegion *webRegion = [[WebDashboardRegion alloc] initWithRect:snappedIntRect(region.bounds) clip:snappedIntRect(region.clip) type:type];
        [regionValues addObject:webRegion];
        [webRegion release];
    }

    [self _addScrollerDashboardRegions:webRegions];

    return webRegions;
}

- (void)_setDashboardBehavior:(WebDashboardBehavior)behavior to:(BOOL)flag
{
    // FIXME: Remove this blanket assignment once Dashboard and Dashcode implement 
    // specific support for the backward compatibility mode flag.
    if (behavior == WebDashboardBehaviorAllowWheelScrolling && flag == NO && _private->page)
        _private->page->settings().setUsesDashboardBackwardCompatibilityMode(true);
    
    switch (behavior) {
        case WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows: {
            _private->dashboardBehaviorAlwaysSendMouseEventsToAllWindows = flag;
            break;
        }
        case WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns: {
            _private->dashboardBehaviorAlwaysSendActiveNullEventsToPlugIns = flag;
            break;
        }
        case WebDashboardBehaviorAlwaysAcceptsFirstMouse: {
            _private->dashboardBehaviorAlwaysAcceptsFirstMouse = flag;
            break;
        }
        case WebDashboardBehaviorAllowWheelScrolling: {
            _private->dashboardBehaviorAllowWheelScrolling = flag;
            break;
        }
        case WebDashboardBehaviorUseBackwardCompatibilityMode: {
            if (_private->page)
                _private->page->settings().setUsesDashboardBackwardCompatibilityMode(flag);
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
            RuntimeEnabledFeatures::sharedFeatures().setLegacyCSSVendorPrefixesEnabled(flag);
#endif
            break;
        }
    }

    // Pre-HTML5 parser quirks should be enabled if Dashboard is in backward
    // compatibility mode. See <rdar://problem/8175982>.
    if (_private->page)
        _private->page->settings().setUsePreHTML5ParserQuirks([self _needsPreHTML5ParserQuirks]);
}

- (BOOL)_dashboardBehavior:(WebDashboardBehavior)behavior
{
    switch (behavior) {
        case WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows: {
            return _private->dashboardBehaviorAlwaysSendMouseEventsToAllWindows;
        }
        case WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns: {
            return _private->dashboardBehaviorAlwaysSendActiveNullEventsToPlugIns;
        }
        case WebDashboardBehaviorAlwaysAcceptsFirstMouse: {
            return _private->dashboardBehaviorAlwaysAcceptsFirstMouse;
        }
        case WebDashboardBehaviorAllowWheelScrolling: {
            return _private->dashboardBehaviorAllowWheelScrolling;
        }
        case WebDashboardBehaviorUseBackwardCompatibilityMode: {
            return _private->page && _private->page->settings().usesDashboardBackwardCompatibilityMode();
        }
    }
    return NO;
}

#endif /* ENABLE(DASHBOARD_SUPPORT) */

+ (void)_setShouldUseFontSmoothing:(BOOL)f
{
    FontCascade::setShouldUseSmoothing(f);
}

+ (BOOL)_shouldUseFontSmoothing
{
    return FontCascade::shouldUseSmoothing();
}

#if !PLATFORM(IOS)
+ (void)_setUsesTestModeFocusRingColor:(BOOL)f
{
    setUsesTestModeFocusRingColor(f);
}

+ (BOOL)_usesTestModeFocusRingColor
{
    return usesTestModeFocusRingColor();
}
#endif

#if PLATFORM(IOS)
- (void)_setUIKitDelegate:(id)delegate
{
    _private->UIKitDelegate = delegate;
    [_private->UIKitDelegateForwarder clearTarget];
    [_private->UIKitDelegateForwarder release];
    _private->UIKitDelegateForwarder = nil;
}

- (id)_UIKitDelegate
{
    return _private->UIKitDelegate;
}

- (void)setWebMailDelegate:(id)delegate
{
    _private->WebMailDelegate = delegate;
}

- (id)_webMailDelegate
{
    return _private->WebMailDelegate;
}

- (id <WebCaretChangeListener>)caretChangeListener
{
    return _private->_caretChangeListener;
}

- (void)setCaretChangeListener:(id <WebCaretChangeListener>)listener
{
    _private->_caretChangeListener = listener;
}

- (NSSet *)caretChangeListeners
{
    return _private->_caretChangeListeners;
}

- (void)addCaretChangeListener:(id <WebCaretChangeListener>)listener
{
    if (_private->_caretChangeListeners == nil) {
        _private->_caretChangeListeners = [[NSMutableSet alloc] init];
    }
    
    [_private->_caretChangeListeners addObject:listener];
}

- (void)removeCaretChangeListener:(id <WebCaretChangeListener>)listener
{
    [_private->_caretChangeListeners removeObject:listener];
}

- (void)removeAllCaretChangeListeners
{
    [_private->_caretChangeListeners removeAllObjects];
}

- (void)caretChanged
{
    [_private->_caretChangeListener caretChanged];
    NSSet *copy = [_private->_caretChangeListeners copy];
    for (id <WebCaretChangeListener> listener in copy) {
        [listener caretChanged];
    }
    [copy release];
}

- (void)_clearDelegates
{
    ASSERT(WebThreadIsLocked() || !WebThreadIsEnabled());
    
    [self _setFormDelegate:nil];
    [self _setUIKitDelegate:nil];
    [self setCaretChangeListener:nil];
    [self removeAllCaretChangeListeners];
    [self setWebMailDelegate:nil];
    
    [self setDownloadDelegate:nil];
    [self setEditingDelegate:nil]; 
    [self setFrameLoadDelegate:nil];
    [self setPolicyDelegate:nil];
    [self setResourceLoadDelegate:nil];
    [self setScriptDebugDelegate:nil];
    [self setUIDelegate:nil];     
}

- (NSURL *)_displayURL
{
    WebThreadLock();
    WebFrame *frame = [self mainFrame];
    // FIXME: <rdar://problem/6362369> We used to get provisionalDataSource here if in provisional state; how do we tell that now? Is this right?
    WebDataSource *dataSource = [frame provisionalDataSource];
    if (!dataSource)
        dataSource = [frame dataSource];
    NSURL *unreachableURL = [dataSource unreachableURL];
    NSURL *URL = unreachableURL != nil ? unreachableURL : [[dataSource request] URL];
    [[URL retain] autorelease];
    return URL;
}
#endif // PLATFORM(IOS)

#if !PLATFORM(IOS)
- (void)setAlwaysShowVerticalScroller:(BOOL)flag
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    if (flag) {
        [scrollview setVerticalScrollingMode:ScrollbarAlwaysOn andLock:YES];
    } else {
        [scrollview setVerticalScrollingModeLocked:NO];
        [scrollview setVerticalScrollingMode:ScrollbarAuto andLock:NO];
    }
}

- (BOOL)alwaysShowVerticalScroller
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    return [scrollview verticalScrollingModeLocked] && [scrollview verticalScrollingMode] == ScrollbarAlwaysOn;
}

- (void)setAlwaysShowHorizontalScroller:(BOOL)flag
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    if (flag) {
        [scrollview setHorizontalScrollingMode:ScrollbarAlwaysOn andLock:YES];
    } else {
        [scrollview setHorizontalScrollingModeLocked:NO];
        [scrollview setHorizontalScrollingMode:ScrollbarAuto andLock:NO];
    }
}

- (void)setProhibitsMainFrameScrolling:(BOOL)prohibits
{
    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->view()->setProhibitsScrolling(prohibits);
}

- (BOOL)alwaysShowHorizontalScroller
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    return [scrollview horizontalScrollingModeLocked] && [scrollview horizontalScrollingMode] == ScrollbarAlwaysOn;
}
#endif // !PLATFORM(IOS)

- (void)_setUseFastImageScalingMode:(BOOL)flag
{
    if (_private->page && _private->page->inLowQualityImageInterpolationMode() != flag) {
        _private->page->setInLowQualityImageInterpolationMode(flag);
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)_inFastImageScalingMode
{
    if (_private->page)
        return _private->page->inLowQualityImageInterpolationMode();
    return NO;
}

- (BOOL)_cookieEnabled
{
    if (_private->page)
        return _private->page->settings().cookieEnabled();
    return YES;
}

- (void)_setCookieEnabled:(BOOL)enable
{
    if (_private->page)
        _private->page->settings().setCookieEnabled(enable);
}

#if !PLATFORM(IOS)
- (void)_setAdditionalWebPlugInPaths:(NSArray *)newPaths
{
    if (!_private->pluginDatabase)
        _private->pluginDatabase = [[WebPluginDatabase alloc] init];
        
    [_private->pluginDatabase setPlugInPaths:newPaths];
    [_private->pluginDatabase refresh];
}
#endif

#if PLATFORM(IOS)
- (BOOL)_locked_plugInsAreRunningInFrame:(WebFrame *)frame
{
    // Ask plug-ins in this frame
    id <WebDocumentView> documentView = [[frame frameView] documentView];
    if (documentView && [documentView isKindOfClass:[WebHTMLView class]]) {
        if ([[(WebHTMLView *)documentView _pluginController] plugInsAreRunning])
            return YES;
    }

    // Ask plug-ins in child frames
    NSArray *childFrames = [frame childFrames];
    unsigned childCount = [childFrames count];
    unsigned childIndex;
    for (childIndex = 0; childIndex < childCount; childIndex++) {
        if ([self _locked_plugInsAreRunningInFrame:[childFrames objectAtIndex:childIndex]])
            return YES;
    }
    
    return NO;
}

- (BOOL)_pluginsAreRunning
{
    WebThreadLock();
    return [self _locked_plugInsAreRunningInFrame:[self mainFrame]];
}

- (void)_locked_recursivelyPerformPlugInSelector:(SEL)selector inFrame:(WebFrame *)frame
{
    // Send the message to plug-ins in this frame
    id <WebDocumentView> documentView = [[frame frameView] documentView];
    if (documentView && [documentView isKindOfClass:[WebHTMLView class]])
        [[(WebHTMLView *)documentView _pluginController] performSelector:selector];

    // Send the message to plug-ins in child frames
    NSArray *childFrames = [frame childFrames];
    unsigned childCount = [childFrames count];
    unsigned childIndex;
    for (childIndex = 0; childIndex < childCount; childIndex++) {
        [self _locked_recursivelyPerformPlugInSelector:selector inFrame:[childFrames objectAtIndex:childIndex]];
    }
}

- (void)_destroyAllPlugIns
{
    WebThreadLock();
    [self _locked_recursivelyPerformPlugInSelector:@selector(destroyAllPlugins) inFrame:[self mainFrame]];
}

- (void)_clearBackForwardCache
{
    WebThreadRun(^{
        WebKit::MemoryMeasure measurer("[WebView _clearBackForwardCache]");
        BackForwardList* bfList = core([self backForwardList]);
        if (!bfList)
            return;

        BOOL didClearBFCache = bfList->clearAllPageCaches();
        if (WebKit::MemoryMeasure::isLoggingEnabled())
            NSLog(@"[WebView _clearBackForwardCache] %@ empty cache\n", (didClearBFCache ? @"DID" : @"did NOT"));
    });
}

- (void)_startAllPlugIns
{
    WebThreadLock();
    [self _locked_recursivelyPerformPlugInSelector:@selector(startAllPlugins) inFrame:[self mainFrame]];
}

- (void)_stopAllPlugIns
{
    WebThreadLock();
    [self _locked_recursivelyPerformPlugInSelector:@selector(stopAllPlugins) inFrame:[self mainFrame]];
}

- (void)_stopAllPlugInsForPageCache
{
    WebThreadLock();
    [self _locked_recursivelyPerformPlugInSelector:@selector(stopPluginsForPageCache) inFrame:[self mainFrame]];
}

- (void)_restorePlugInsFromCache
{
    WebThreadLock();
    [self _locked_recursivelyPerformPlugInSelector:@selector(restorePluginsFromCache) inFrame:[self mainFrame]];
}

- (BOOL)_setMediaLayer:(CALayer*)layer forPluginView:(NSView*)pluginView
{
    WebThreadLock();

    Frame* mainCoreFrame = [self _mainCoreFrame];
    for (Frame* frame = mainCoreFrame; frame; frame = frame->tree().traverseNext()) {
        FrameView* coreView = frame ? frame->view() : 0;
        if (!coreView)
            continue;

        // Get the GraphicsLayer for this widget.
        GraphicsLayer* layerForWidget = coreView->graphicsLayerForPlatformWidget(pluginView);
        if (!layerForWidget)
            continue;
        
        if (layerForWidget->contentsLayerForMedia() != layer) {
            layerForWidget->setContentsToPlatformLayer(layer, GraphicsLayer::ContentsLayerForMedia);
            // We need to make sure the layer hierachy change is applied immediately.
            if (mainCoreFrame->view())
                mainCoreFrame->view()->flushCompositingStateIncludingSubframes();
            return YES;
        }
    }

    return NO;
}

- (void)_setNeedsUnrestrictedGetMatchedCSSRules:(BOOL)flag
{
    if (_private->page)
        _private->page->settings().setCrossOriginCheckInGetMatchedCSSRulesDisabled(flag);
}
#endif // PLATFORM(IOS)

- (void)_attachScriptDebuggerToAllFrames
{
    for (Frame* frame = [self _mainCoreFrame]; frame; frame = frame->tree().traverseNext())
        [kit(frame) _attachScriptDebugger];
}

- (void)_detachScriptDebuggerFromAllFrames
{
    for (Frame* frame = [self _mainCoreFrame]; frame; frame = frame->tree().traverseNext())
        [kit(frame) _detachScriptDebugger];
}

#if !PLATFORM(IOS)
- (void)setBackgroundColor:(NSColor *)backgroundColor
{
    if ([_private->backgroundColor isEqual:backgroundColor])
        return;

    id old = _private->backgroundColor;
    _private->backgroundColor = [backgroundColor retain];
    [old release];

    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (NSColor *)backgroundColor
{
    return _private->backgroundColor;
}
#else
- (void)setBackgroundColor:(CGColorRef)backgroundColor
{
   if (!backgroundColor || CFEqual(_private->backgroundColor, backgroundColor))
        return;

    CFTypeRef old = _private->backgroundColor;
    CFRetain(backgroundColor);
    _private->backgroundColor = backgroundColor;
    CFRelease(old);

    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (CGColorRef)backgroundColor
{
    return _private->backgroundColor;
}
#endif

- (BOOL)defersCallbacks
{
    if (!_private->page)
        return NO;
    return _private->page->defersLoading();
}

- (void)setDefersCallbacks:(BOOL)defer
{
    if (!_private->page)
        return;
    return _private->page->setDefersLoading(defer);
}

#if TARGET_OS_IPHONE && USE(QUICK_LOOK)
- (NSDictionary *)quickLookContentForURL:(NSURL *)url
{
    NSString *uti = qlPreviewConverterUTIForURL(url);
    if (!uti)
        return nil;

    NSString *fileName = qlPreviewConverterFileNameForURL(url);
    if (!fileName)
        return nil;

    return [NSDictionary dictionaryWithObjectsAndKeys: fileName, WebQuickLookFileNameKey, uti, WebQuickLookUTIKey, nil];
}
#endif

#if PLATFORM(IOS)
- (BOOL)_isStopping
{
    return _private->isStopping;
}

- (BOOL)_isClosing
{
    return _private->closing;
}

+ (NSArray *)_productivityDocumentMIMETypes
{
#if USE(QUICK_LOOK)
    return [QLPreviewGetSupportedMIMETypesSet() allObjects];
#else
    return nil;
#endif
}

- (void)_setAllowsMessaging:(BOOL)aFlag
{
    _private->allowsMessaging = aFlag;
}

- (BOOL)_allowsMessaging
{
    return _private->allowsMessaging;
}

- (void)_setFixedLayoutSize:(CGSize)size
{
    ASSERT(WebThreadIsLocked());
    _private->fixedLayoutSize = size;
    if (Frame* mainFrame = core([self mainFrame])) {
        IntSize newSize(size);
        mainFrame->view()->setFixedLayoutSize(newSize);
        mainFrame->view()->setUseFixedLayout(!newSize.isEmpty());
        [self setNeedsDisplay:YES];
    }
}

- (CGSize)_fixedLayoutSize
{
    return _private->fixedLayoutSize;
}

- (void)_synchronizeCustomFixedPositionLayoutRect
{
    ASSERT(WebThreadIsLocked());

    IntRect newRect;
    {
        MutexLocker locker(_private->pendingFixedPositionLayoutRectMutex);
        if (CGRectIsNull(_private->pendingFixedPositionLayoutRect))
            return;
        newRect = enclosingIntRect(_private->pendingFixedPositionLayoutRect);
        _private->pendingFixedPositionLayoutRect = CGRectNull;
    }

    if (Frame* mainFrame = core([self mainFrame]))
        mainFrame->view()->setCustomFixedPositionLayoutRect(newRect);
}

- (void)_setCustomFixedPositionLayoutRectInWebThread:(CGRect)rect synchronize:(BOOL)synchronize
{
    {
        MutexLocker locker(_private->pendingFixedPositionLayoutRectMutex);
        _private->pendingFixedPositionLayoutRect = rect;
    }
    if (!synchronize)
        return;
    WebThreadRun(^{
        [self _synchronizeCustomFixedPositionLayoutRect];
    });
}

- (void)_setCustomFixedPositionLayoutRect:(CGRect)rect
{
    ASSERT(WebThreadIsLocked());
    {
        MutexLocker locker(_private->pendingFixedPositionLayoutRectMutex);
        _private->pendingFixedPositionLayoutRect = rect;
    }
    [self _synchronizeCustomFixedPositionLayoutRect];
}

- (BOOL)_fetchCustomFixedPositionLayoutRect:(NSRect*)rect
{
    MutexLocker locker(_private->pendingFixedPositionLayoutRectMutex);
    if (CGRectIsNull(_private->pendingFixedPositionLayoutRect))
        return false;

    *rect = _private->pendingFixedPositionLayoutRect;
    _private->pendingFixedPositionLayoutRect = CGRectNull;
    return true;
}

- (void)_viewGeometryDidChange
{
    ASSERT(WebThreadIsLocked());
    
    if (Frame* coreFrame = [self _mainCoreFrame])
        coreFrame->viewportOffsetChanged(Frame::IncrementalScrollOffset);
}

- (void)_overflowScrollPositionChangedTo:(CGPoint)offset forNode:(DOMNode *)domNode isUserScroll:(BOOL)userScroll
{
    // Find the frame
    Node* node = core(domNode);
    Frame* frame = node->document().frame();
    if (!frame)
        return;

    frame->overflowScrollPositionChangedForNode(roundedIntPoint(offset), node, userScroll);
}

+ (void)_doNotStartObservingNetworkReachability
{
    Settings::setShouldOptOutOfNetworkStateObservation(true);
}
#endif // PLATFORM(IOS)

#if ENABLE(TOUCH_EVENTS)
- (NSArray *)_touchEventRegions
{
    Frame* frame = [self _mainCoreFrame];
    if (!frame)
        return nil;
    
    Document* document = frame->document();
    if (!document)
        return nil;

    Vector<IntRect> rects;
    document->getTouchRects(rects);

    if (rects.size() == 0)
        return nil;

    NSMutableArray *eventRegionArray = [[[NSMutableArray alloc] initWithCapacity:rects.size()] autorelease];

    Vector<IntRect>::const_iterator end = rects.end();
    for (Vector<IntRect>::const_iterator it = rects.begin(); it != end; ++it) {
        const IntRect& rect = *it;
        if (rect.isEmpty())
            continue;

        // Note that these rectangles are in the coordinate system of the document (inside the WebHTMLView), which is not
        // the same as the coordinate system of the WebView. If you want to do comparisons with locations in the WebView,
        // you must convert between the two using WAKView's convertRect:toView: selector. This will take care of scaling
        // and translations (which are relevant for right-to-left column layout).

        // The event region wants this points in this order:
        //  p2------p3
        //  |       |
        //  p1------p4
        //
        WebEventRegion *eventRegion = [[WebEventRegion alloc] initWithPoints:FloatPoint(rect.x(), rect.maxY())
                                                                            :FloatPoint(rect.x(), rect.y())
                                                                            :FloatPoint(rect.maxX(), rect.y())
                                                                            :FloatPoint(rect.maxX(), rect.maxY())];
        if (eventRegion) {
            [eventRegionArray addObject:eventRegion];
            [eventRegion release];
        }
    }

    return eventRegionArray;
}
#endif // ENABLE(TOUCH_EVENTS)

// For backwards compatibility with the WebBackForwardList API, we honor both
// a per-WebView and a per-preferences setting for whether to use the page cache.

- (BOOL)usesPageCache
{
    return _private->usesPageCache && [[self preferences] usesPageCache];
}

- (void)setUsesPageCache:(BOOL)usesPageCache
{
    _private->usesPageCache = usesPageCache;

    // Update our own settings and post the public notification only
    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];
}

- (WebHistoryItem *)_globalHistoryItem
{
    if (!_private)
        return nil;

    return kit(_private->_globalHistoryItem.get());
}

- (void)_setGlobalHistoryItem:(HistoryItem*)historyItem
{
    _private->_globalHistoryItem = historyItem;
}

- (WebTextIterator *)textIteratorForRect:(NSRect)rect
{
    IntPoint rectStart(rect.origin.x, rect.origin.y);
    IntPoint rectEnd(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height);
    
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    
    VisibleSelection selectionInsideRect(coreFrame->visiblePositionForPoint(rectStart), coreFrame->visiblePositionForPoint(rectEnd));
    
    return [[[WebTextIterator alloc] initWithRange:kit(selectionInsideRect.toNormalizedRange().get())] autorelease];
}

#if ENABLE(DASHBOARD_SUPPORT)
- (void)handleAuthenticationForResource:(id)identifier challenge:(NSURLAuthenticationChallenge *)challenge fromDataSource:(WebDataSource *)dataSource 
{
    NSWindow *window = [self hostWindow] ? [self hostWindow] : [self window]; 
    [[WebPanelAuthenticationHandler sharedHandler] startAuthentication:challenge window:window]; 
} 
#endif

#if !PLATFORM(IOS)
- (void)_clearUndoRedoOperations
{
    if (!_private->page)
        return;
    _private->page->clearUndoRedoOperations();
}
#endif

- (void)_executeCoreCommandByName:(NSString *)name value:(NSString *)value
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;
    coreFrame->editor().command(name).execute(value);
}

- (void)_clearMainFrameName
{
    _private->page->mainFrame().tree().clearName();
}

- (void)setSelectTrailingWhitespaceEnabled:(BOOL)flag
{
    if (_private->page->settings().selectTrailingWhitespaceEnabled() != flag) {
        _private->page->settings().setSelectTrailingWhitespaceEnabled(flag);
        [self setSmartInsertDeleteEnabled:!flag];
    }
}

- (BOOL)isSelectTrailingWhitespaceEnabled
{
    return _private->page->settings().selectTrailingWhitespaceEnabled();
}

- (void)setMemoryCacheDelegateCallsEnabled:(BOOL)enabled
{
    _private->page->setMemoryCacheClientCallsEnabled(enabled);
}

- (BOOL)areMemoryCacheDelegateCallsEnabled
{
    return _private->page->areMemoryCacheClientCallsEnabled();
}

#if !PLATFORM(IOS)
+ (NSCursor *)_pointingHandCursor
{
    return handCursor().platformCursor();
}
#endif

- (BOOL)_postsAcceleratedCompositingNotifications
{
    return _private->postsAcceleratedCompositingNotifications;
}
- (void)_setPostsAcceleratedCompositingNotifications:(BOOL)flag
{
    _private->postsAcceleratedCompositingNotifications = flag;
}

- (BOOL)_isUsingAcceleratedCompositing
{
    Frame* coreFrame = [self _mainCoreFrame];
    for (Frame* frame = coreFrame; frame; frame = frame->tree().traverseNext(coreFrame)) {
        NSView *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]] && [(WebHTMLView *)documentView _isUsingAcceleratedCompositing])
            return YES;
    }

    return NO;
}

- (void)_setBaseCTM:(CGAffineTransform)transform forContext:(CGContextRef)context
{
    WKSetBaseCTM(context, transform);
}

- (BOOL)interactiveFormValidationEnabled
{
    return _private->interactiveFormValidationEnabled;
}

- (void)setInteractiveFormValidationEnabled:(BOOL)enabled
{
    _private->interactiveFormValidationEnabled = enabled;
}

- (int)validationMessageTimerMagnification
{
    return _private->validationMessageTimerMagnification;
}

- (void)setValidationMessageTimerMagnification:(int)newValue
{
    _private->validationMessageTimerMagnification = newValue;
}

- (BOOL)_isSoftwareRenderable
{
    Frame* coreFrame = [self _mainCoreFrame];
    for (Frame* frame = coreFrame; frame; frame = frame->tree().traverseNext(coreFrame)) {
        if (FrameView* view = frame->view()) {
            if (!view->isSoftwareRenderable())
                return NO;
        }
    }

    return YES;
}

- (void)_setIncludesFlattenedCompositingLayersWhenDrawingToBitmap:(BOOL)flag
{
    _private->includesFlattenedCompositingLayersWhenDrawingToBitmap = flag;
}

- (BOOL)_includesFlattenedCompositingLayersWhenDrawingToBitmap
{
    return _private->includesFlattenedCompositingLayersWhenDrawingToBitmap;
}

- (void)setTracksRepaints:(BOOL)flag
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        view->setTracksRepaints(flag);
}

- (BOOL)isTrackingRepaints
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        return view->isTrackingRepaints();
    
    return NO;
}

- (void)resetTrackedRepaints
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        view->resetTrackedRepaints();
}

- (NSArray*)trackedRepaintRects
{
    Frame* coreFrame = [self _mainCoreFrame];
    FrameView* view = coreFrame->view();
    if (!view || !view->isTrackingRepaints())
        return nil;

    const Vector<FloatRect>& repaintRects = view->trackedRepaintRects();
    NSMutableArray* rectsArray = [[NSMutableArray alloc] initWithCapacity:repaintRects.size()];
    
    for (unsigned i = 0; i < repaintRects.size(); ++i)
        [rectsArray addObject:[NSValue valueWithRect:snappedIntRect(LayoutRect(repaintRects[i]))]];

    return [rectsArray autorelease];
}

#if !PLATFORM(IOS)
- (NSPasteboard *)_insertionPasteboard
{
    return _private ? _private->insertionPasteboard : nil;
}
#endif

+ (void)_addOriginAccessWhitelistEntryWithSourceOrigin:(NSString *)sourceOrigin destinationProtocol:(NSString *)destinationProtocol destinationHost:(NSString *)destinationHost allowDestinationSubdomains:(BOOL)allowDestinationSubdomains
{
    SecurityPolicy::addOriginAccessWhitelistEntry(SecurityOrigin::createFromString(sourceOrigin).get(), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

+ (void)_removeOriginAccessWhitelistEntryWithSourceOrigin:(NSString *)sourceOrigin destinationProtocol:(NSString *)destinationProtocol destinationHost:(NSString *)destinationHost allowDestinationSubdomains:(BOOL)allowDestinationSubdomains
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(SecurityOrigin::createFromString(sourceOrigin).get(), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

+ (void)_resetOriginAccessWhitelists
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

- (BOOL)_isViewVisible
{
    if (![self window])
        return false;

    if (![[self window] isVisible])
        return false;

    if ([self isHiddenOrHasHiddenAncestor])
        return false;

    return true;
}

- (void)_updateVisibilityState
{
    if (_private && _private->page)
        [self _setIsVisible:[self _isViewVisible]];
}

- (void)_updateActiveState
{
    if (_private && _private->page)
#if PLATFORM(IOS)
        _private->page->focusController().setActive([[self window] isKeyWindow]);
#else
        _private->page->focusController().setActive([[self window] _hasKeyAppearance]);
#endif
}

static Vector<String> toStringVector(NSArray* patterns)
{
    Vector<String> patternsVector;

    NSUInteger count = [patterns count];
    if (!count)
        return patternsVector;

    for (NSUInteger i = 0; i < count; ++i) {
        id entry = [patterns objectAtIndex:i];
        if ([entry isKindOfClass:[NSString class]])
            patternsVector.append(String((NSString *)entry));
    }
    return patternsVector;
}

+ (void)_addUserScriptToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                    whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                injectionTime:(WebUserScriptInjectionTime)injectionTime
{
    [WebView _addUserScriptToGroup:groupName world:world source:source url:url whitelist:whitelist blacklist:blacklist injectionTime:injectionTime injectedFrames:WebInjectInAllFrames];
}

+ (void)_addUserScriptToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                    whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                injectionTime:(WebUserScriptInjectionTime)injectionTime
               injectedFrames:(WebUserContentInjectedFrames)injectedFrames
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto viewGroup = WebViewGroup::getOrCreate(groupName, String());
    if (!viewGroup)
        return;

    if (!world)
        return;

    auto userScript = std::make_unique<UserScript>(source, url, toStringVector(whitelist), toStringVector(blacklist), injectionTime == WebInjectAtDocumentStart ? InjectAtDocumentStart : InjectAtDocumentEnd, injectedFrames == WebInjectInAllFrames ? InjectInAllFrames : InjectInTopFrameOnly);
    viewGroup->userContentController().addUserScript(*core(world), WTF::move(userScript));
}

+ (void)_addUserStyleSheetToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                        whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
{
    [WebView _addUserStyleSheetToGroup:groupName world:world source:source url:url whitelist:whitelist blacklist:blacklist injectedFrames:WebInjectInAllFrames];
}

+ (void)_addUserStyleSheetToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                        whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                   injectedFrames:(WebUserContentInjectedFrames)injectedFrames
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto viewGroup = WebViewGroup::getOrCreate(groupName, String());
    if (!viewGroup)
        return;

    if (!world)
        return;

    auto styleSheet = std::make_unique<UserStyleSheet>(source, url, toStringVector(whitelist), toStringVector(blacklist), injectedFrames == WebInjectInAllFrames ? InjectInAllFrames : InjectInTopFrameOnly, UserStyleUserLevel);
    viewGroup->userContentController().addUserStyleSheet(*core(world), WTF::move(styleSheet), InjectInExistingDocuments);
}

+ (void)_removeUserScriptFromGroup:(NSString *)groupName world:(WebScriptWorld *)world url:(NSURL *)url
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto* viewGroup = WebViewGroup::get(group);
    if (!viewGroup)
        return;

    if (!world)
        return;

    viewGroup->userContentController().removeUserScript(*core(world), url);
}

+ (void)_removeUserStyleSheetFromGroup:(NSString *)groupName world:(WebScriptWorld *)world url:(NSURL *)url
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto* viewGroup = WebViewGroup::get(group);
    if (!viewGroup)
        return;

    if (!world)
        return;

    viewGroup->userContentController().removeUserStyleSheet(*core(world), url);
}

+ (void)_removeUserScriptsFromGroup:(NSString *)groupName world:(WebScriptWorld *)world
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto* viewGroup = WebViewGroup::get(group);
    if (!viewGroup)
        return;

    if (!world)
        return;

    viewGroup->userContentController().removeUserScripts(*core(world));
}

+ (void)_removeUserStyleSheetsFromGroup:(NSString *)groupName world:(WebScriptWorld *)world
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto* viewGroup = WebViewGroup::get(group);
    if (!viewGroup)
        return;

    if (!world)
        return;

    viewGroup->userContentController().removeUserStyleSheets(*core(world));
}

+ (void)_removeAllUserContentFromGroup:(NSString *)groupName
{
    String group(groupName);
    if (group.isEmpty())
        return;

    auto* viewGroup = WebViewGroup::get(group);
    if (!viewGroup)
        return;

    viewGroup->userContentController().removeAllUserContent();
}

- (BOOL)allowsNewCSSAnimationsWhileSuspended
{
    Frame* frame = core([self mainFrame]);
    if (frame)
        return frame->animation().allowsNewAnimationsWhileSuspended();

    return false;
}

- (void)setAllowsNewCSSAnimationsWhileSuspended:(BOOL)allowed
{
    Frame* frame = core([self mainFrame]);
    if (frame)
        frame->animation().setAllowsNewAnimationsWhileSuspended(allowed);
}

- (BOOL)cssAnimationsSuspended
{
    // should ask the page!
    Frame* frame = core([self mainFrame]);
    if (frame)
        return frame->animation().isSuspended();

    return false;
}

- (void)setCSSAnimationsSuspended:(BOOL)suspended
{
    Frame* frame = core([self mainFrame]);
    if (suspended == frame->animation().isSuspended())
        return;
        
    if (suspended)
        frame->animation().suspendAnimations();
    else
        frame->animation().resumeAnimations();
}

+ (void)_setDomainRelaxationForbidden:(BOOL)forbidden forURLScheme:(NSString *)scheme
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}

+ (void)_registerURLSchemeAsSecure:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsSecure(scheme);
}

+ (void)_registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(scheme);
}

+ (void)_registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(scheme);
}

- (void)_scaleWebView:(float)scale atOrigin:(NSPoint)origin
{
    _private->page->setPageScaleFactor(scale, IntPoint(origin));
}

- (float)_viewScaleFactor
{
    return _private->page->pageScaleFactor();
}

- (void)_setUseFixedLayout:(BOOL)fixed
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    view->setUseFixedLayout(fixed);
    if (!fixed)
        view->setFixedLayoutSize(IntSize());
}

#if !PLATFORM(IOS)
- (void)_setFixedLayoutSize:(NSSize)size
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;
    
    FrameView* view = coreFrame->view();
    if (!view)
        return;
    
    view->setFixedLayoutSize(IntSize(size));
    view->forceLayout();
}
#endif

- (BOOL)_useFixedLayout
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return NO;
    
    FrameView* view = coreFrame->view();
    if (!view)
        return NO;
    
    return view->useFixedLayout();
}

#if !PLATFORM(IOS)
- (NSSize)_fixedLayoutSize
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return IntSize();
    
    FrameView* view = coreFrame->view();
    if (!view)
        return IntSize();

    return view->fixedLayoutSize();
}
#endif

- (void)_setPaginationMode:(WebPaginationMode)paginationMode
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    switch (paginationMode) {
    case WebPaginationModeUnpaginated:
        pagination.mode = Pagination::Unpaginated;
        break;
    case WebPaginationModeLeftToRight:
        pagination.mode = Pagination::LeftToRightPaginated;
        break;
    case WebPaginationModeRightToLeft:
        pagination.mode = Pagination::RightToLeftPaginated;
        break;
    case WebPaginationModeTopToBottom:
        pagination.mode = Pagination::TopToBottomPaginated;
        break;
    case WebPaginationModeBottomToTop:
        pagination.mode = Pagination::BottomToTopPaginated;
        break;
    default:
        return;
    }

    page->setPagination(pagination);
}

- (WebPaginationMode)_paginationMode
{
    Page* page = core(self);
    if (!page)
        return WebPaginationModeUnpaginated;

    switch (page->pagination().mode) {
    case Pagination::Unpaginated:
        return WebPaginationModeUnpaginated;
    case Pagination::LeftToRightPaginated:
        return WebPaginationModeLeftToRight;
    case Pagination::RightToLeftPaginated:
        return WebPaginationModeRightToLeft;
    case Pagination::TopToBottomPaginated:
        return WebPaginationModeTopToBottom;
    case Pagination::BottomToTopPaginated:
        return WebPaginationModeBottomToTop;
    }

    ASSERT_NOT_REACHED();
    return WebPaginationModeUnpaginated;
}

- (void)_listenForLayoutMilestones:(WebLayoutMilestones)layoutMilestones
{
    Page* page = core(self);
    if (!page)
        return;

    page->addLayoutMilestones(coreLayoutMilestones(layoutMilestones));
}

- (WebLayoutMilestones)_layoutMilestones
{
    Page* page = core(self);
    if (!page)
        return 0;

    return kitLayoutMilestones(page->requestedLayoutMilestones());
}

- (WebPageVisibilityState)_visibilityState
{
    if (_private->page)
        return kit(_private->page->visibilityState());
    return WebPageVisibilityStateVisible;
}

- (void)_setIsVisible:(BOOL)isVisible
{
    if (_private->page)
        _private->page->setIsVisible(isVisible);
}

- (void)_setVisibilityState:(WebPageVisibilityState)visibilityState isInitialState:(BOOL)isInitialState
{
    UNUSED_PARAM(isInitialState);

    if (_private->page) {
        _private->page->setIsVisible(visibilityState == WebPageVisibilityStateVisible);
        if (visibilityState == WebPageVisibilityStatePrerender)
            _private->page->setIsPrerender();
    }
}

- (void)_setPaginationBehavesLikeColumns:(BOOL)behavesLikeColumns
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.behavesLikeColumns = behavesLikeColumns;

    page->setPagination(pagination);
}

- (BOOL)_paginationBehavesLikeColumns
{
    Page* page = core(self);
    if (!page)
        return NO;

    return page->pagination().behavesLikeColumns;
}

- (void)_setPageLength:(CGFloat)pageLength
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.pageLength = pageLength;

    page->setPagination(pagination);
}

- (CGFloat)_pageLength
{
    Page* page = core(self);
    if (!page)
        return 1;

    return page->pagination().pageLength;
}

- (void)_setGapBetweenPages:(CGFloat)pageGap
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.gap = pageGap;
    page->setPagination(pagination);
}

- (CGFloat)_gapBetweenPages
{
    Page* page = core(self);
    if (!page)
        return 0;

    return page->pagination().gap;
}

- (NSUInteger)_pageCount
{
    Page* page = core(self);
    if (!page)
        return 0;

    return page->pageCount();
}

#if !PLATFORM(IOS)
- (CGFloat)_backingScaleFactor
{
    return [self _deviceScaleFactor];
}

- (void)_setCustomBackingScaleFactor:(CGFloat)customScaleFactor
{
    float oldScaleFactor = [self _deviceScaleFactor];

    _private->customDeviceScaleFactor = customScaleFactor;

    if (oldScaleFactor != [self _deviceScaleFactor])
        _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
}
#endif

- (NSUInteger)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag highlight:(BOOL)highlight limit:(NSUInteger)limit
{
    return [self countMatchesForText:string options:(caseFlag ? 0 : WebFindOptionsCaseInsensitive) highlight:highlight limit:limit markMatches:YES];
}

- (NSUInteger)countMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    return [self countMatchesForText:string options:(caseFlag ? 0 : WebFindOptionsCaseInsensitive) highlight:highlight limit:limit markMatches:markMatches];
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection
{
    return [self findString:string options:((forward ? 0 : WebFindOptionsBackwards) | (caseFlag ? 0 : WebFindOptionsCaseInsensitive) | (wrapFlag ? WebFindOptionsWrapAround : 0) | (startInSelection ? WebFindOptionsStartInSelection : 0))];
}

+ (void)_setLoadResourcesSerially:(BOOL)serialize 
{
    WebPlatformStrategies::initializeIfNecessary();
    resourceLoadScheduler()->setSerialLoadingEnabled(serialize);
}

+ (BOOL)_HTTPPipeliningEnabled
{
    return ResourceRequest::httpPipeliningEnabled();
}

+ (void)_setHTTPPipeliningEnabled:(BOOL)enabled
{
    ResourceRequest::setHTTPPipeliningEnabled(enabled);
}

#if PLATFORM(IOS)
- (WebFixedPositionContent*)_fixedPositionContent
{
    return _private ? _private->_fixedPositionContent : nil;
}

- (void)_documentScaleChanged
{
    if (WebNodeHighlight *currentHighlight = [self currentNodeHighlight])
        [currentHighlight setNeedsDisplay];

    if (_private->indicateLayer) {
        [_private->indicateLayer setNeedsLayout];
        [_private->indicateLayer setNeedsDisplay];
    }
}

- (BOOL)_wantsTelephoneNumberParsing
{
    if (!_private->page)
        return NO;
    return _private->page->settings().telephoneNumberParsingEnabled();
}

- (void)_setWantsTelephoneNumberParsing:(BOOL)flag
{
    if (_private->page)
        _private->page->settings().setTelephoneNumberParsingEnabled(flag);
}

- (BOOL)_webGLEnabled
{
    if (!_private->page)
        return NO;
    return _private->page->settings().webGLEnabled();
}

- (void)_setWebGLEnabled:(BOOL)enabled
{
    if (_private->page)
        _private->page->settings().setWebGLEnabled(enabled);
}

+ (void)_setTileCacheLayerPoolCapacity:(unsigned)capacity
{
    LegacyTileCache::setLayerPoolCapacity(capacity);
}
#endif // PLATFORM(IOS)

- (void)_setSourceApplicationAuditData:(NSData *)sourceApplicationAuditData
{
    if (_private->sourceApplicationAuditData == sourceApplicationAuditData)
        return;

    _private->sourceApplicationAuditData = adoptNS([sourceApplicationAuditData copy]);
}

- (NSData *)_sourceApplicationAuditData
{
    return _private->sourceApplicationAuditData.get();
}

- (void)_setFontFallbackPrefersPictographs:(BOOL)flag
{
    if (_private->page)
        _private->page->settings().setFontFallbackPrefersPictographs(flag);
}

@end

@implementation _WebSafeForwarder

// Used to send messages to delegates that implement informal protocols.

- (instancetype)initWithTarget:(id)t defaultTarget:(id)dt
{
    self = [super init];
    if (!self)
        return nil;
    target = t; // Non retained.
    defaultTarget = dt;
    return self;
}

#if PLATFORM(IOS)
- (id)asyncForwarder
{
    dispatch_once(&asyncForwarderPred, ^{
        asyncForwarder = [[_WebSafeAsyncForwarder alloc] initWithForwarder:self];
    });
    return asyncForwarder;
}

- (void)dealloc
{
    [asyncForwarder release];
    [super dealloc];
}

- (void)clearTarget
{
    target = nil;
}
#endif

- (void)forwardInvocation:(NSInvocation *)invocation
{
#if PLATFORM(IOS)
    if (WebThreadIsCurrent()) {
        [invocation retainArguments];
        WebThreadCallDelegate(invocation);
        return;
    }
#endif
    if ([target respondsToSelector:[invocation selector]]) {
        @try {
            [invocation invokeWithTarget:target];
        } @catch(id exception) {
            ReportDiscardedDelegateException([invocation selector], exception);
        }
        return;
    }

    if ([defaultTarget respondsToSelector:[invocation selector]])
        [invocation invokeWithTarget:defaultTarget];

    // Do nothing quietly if method not implemented.
}

#if PLATFORM(IOS)
- (BOOL)respondsToSelector:(SEL)aSelector
{
    return [defaultTarget respondsToSelector:aSelector];
}
#endif

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    return [defaultTarget methodSignatureForSelector:aSelector];
}

@end

#if PLATFORM(IOS)
@implementation _WebSafeAsyncForwarder

- (id)initWithForwarder:(_WebSafeForwarder *)forwarder
{
    if (!(self = [super init]))
        return nil;
    _forwarder = forwarder;
    return self;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    // Store _forwarder in an ivar so it is retained by the block.
    _WebSafeForwarder *forwarder = _forwarder;
    if (WebThreadIsCurrent()) {
        [invocation retainArguments];
        dispatch_async(dispatch_get_main_queue(), ^{
            [forwarder forwardInvocation:invocation];
        });
    } else
        [forwarder forwardInvocation:invocation];
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    return [_forwarder respondsToSelector:aSelector];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    return [_forwarder methodSignatureForSelector:aSelector];
}

@end
#endif

@implementation WebView

+ (void)initialize
{
    static BOOL initialized = NO;
    if (initialized)
        return;
    initialized = YES;

    InitWebCoreSystemInterface();
#if !PLATFORM(IOS)
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    RunLoop::initializeMainRunLoop();
#endif

#if !PLATFORM(IOS)
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_applicationWillTerminate) name:NSApplicationWillTerminateNotification object:NSApp];
#endif
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_cacheModelChangedNotification:) name:WebPreferencesCacheModelChangedInternalNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesRemovedNotification:) name:WebPreferencesRemovedNotification object:nil];

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey]];

#if PLATFORM(IOS)
    continuousSpellCheckingEnabled = NO;
#endif

#if !PLATFORM(IOS)
    continuousSpellCheckingEnabled = [defaults boolForKey:WebContinuousSpellCheckingEnabled];
    grammarCheckingEnabled = [defaults boolForKey:WebGrammarCheckingEnabled];
#endif

    FontCascade::setDefaultTypesettingFeatures([defaults boolForKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey] ? Kerning | Ligatures : 0);

#if !PLATFORM(IOS)
    automaticQuoteSubstitutionEnabled = [self _shouldAutomaticQuoteSubstitutionBeEnabled];
    automaticLinkDetectionEnabled = [defaults boolForKey:WebAutomaticLinkDetectionEnabled];
    automaticDashSubstitutionEnabled = [self _shouldAutomaticDashSubstitutionBeEnabled];
    automaticTextReplacementEnabled = [self _shouldAutomaticTextReplacementBeEnabled];
    automaticSpellingCorrectionEnabled = [self _shouldAutomaticSpellingCorrectionBeEnabled];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticTextReplacementEnabled:)
        name:NSSpellCheckerDidChangeAutomaticTextReplacementNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticSpellingCorrectionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticSpellingCorrectionNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticQuoteSubstitutionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticQuoteSubstitutionNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticDashSubstitutionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticDashSubstitutionNotification object:nil];
#endif
}

#if !PLATFORM(IOS)
+ (BOOL)_shouldAutomaticTextReplacementBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticTextReplacementEnabled])
        return [NSSpellChecker isAutomaticTextReplacementEnabled];
    return [defaults boolForKey:WebAutomaticTextReplacementEnabled];
}

+ (void)_didChangeAutomaticTextReplacementEnabled:(NSNotification *)notification
{
    automaticTextReplacementEnabled = [self _shouldAutomaticTextReplacementBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (BOOL)_shouldAutomaticSpellingCorrectionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticSpellingCorrectionEnabled])
        return [NSSpellChecker isAutomaticTextReplacementEnabled];
    return [defaults boolForKey:WebAutomaticSpellingCorrectionEnabled];
}

+ (void)_didChangeAutomaticSpellingCorrectionEnabled:(NSNotification *)notification
{
    automaticSpellingCorrectionEnabled = [self _shouldAutomaticSpellingCorrectionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (BOOL)_shouldAutomaticQuoteSubstitutionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticQuoteSubstitutionEnabled])
        return [NSSpellChecker isAutomaticQuoteSubstitutionEnabled];

    return [defaults boolForKey:WebAutomaticQuoteSubstitutionEnabled];
}

+ (BOOL)_shouldAutomaticDashSubstitutionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticDashSubstitutionEnabled])
        return [NSSpellChecker isAutomaticDashSubstitutionEnabled];

    return [defaults boolForKey:WebAutomaticDashSubstitutionEnabled];
}

+ (void)_didChangeAutomaticQuoteSubstitutionEnabled:(NSNotification *)notification
{
    automaticQuoteSubstitutionEnabled = [self _shouldAutomaticQuoteSubstitutionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (void)_didChangeAutomaticDashSubstitutionEnabled:(NSNotification *)notification
{
    automaticDashSubstitutionEnabled = [self _shouldAutomaticDashSubstitutionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (void)_applicationWillTerminate
{   
    applicationIsTerminating = YES;

    if (fastDocumentTeardownEnabled())
        [self closeAllWebViews];

    if (!pluginDatabaseClientCount)
        [WebPluginDatabase closeSharedDatabase];

    WebStorageNamespaceProvider::closeLocalStorage();
}
#endif // !PLATFORM(IOS)

+ (BOOL)_canShowMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins
{
    return [self _viewClass:nil andRepresentationClass:nil forMIMEType:MIMEType allowingPlugins:allowPlugins];
}

+ (BOOL)canShowMIMEType:(NSString *)MIMEType
{
    return [self _canShowMIMEType:MIMEType allowingPlugins:YES];
}

- (BOOL)_canShowMIMEType:(NSString *)MIMEType
{
    return [[self class] _canShowMIMEType:MIMEType allowingPlugins:[_private->preferences arePlugInsEnabled]];
}

- (WebBasePluginPackage *)_pluginForMIMEType:(NSString *)MIMEType
{
    if (![_private->preferences arePlugInsEnabled])
        return nil;

    WebBasePluginPackage *pluginPackage = [[WebPluginDatabase sharedDatabase] pluginForMIMEType:MIMEType];
    if (pluginPackage)
        return pluginPackage;
    
#if !PLATFORM(IOS)
    if (_private->pluginDatabase)
        return [_private->pluginDatabase pluginForMIMEType:MIMEType];
#endif
    
    return nil;
}

- (WebBasePluginPackage *)_pluginForExtension:(NSString *)extension
{
    if (![_private->preferences arePlugInsEnabled])
        return nil;

    WebBasePluginPackage *pluginPackage = [[WebPluginDatabase sharedDatabase] pluginForExtension:extension];
    if (pluginPackage)
        return pluginPackage;
    
#if !PLATFORM(IOS)
    if (_private->pluginDatabase)
        return [_private->pluginDatabase pluginForExtension:extension];
#endif
    
    return nil;
}

#if !PLATFORM(IOS)
- (void)addPluginInstanceView:(NSView *)view
{
    if (!_private->pluginDatabase)
        _private->pluginDatabase = [[WebPluginDatabase alloc] init];
    [_private->pluginDatabase addPluginInstanceView:view];
}

- (void)removePluginInstanceView:(NSView *)view
{
    if (_private->pluginDatabase)
        [_private->pluginDatabase removePluginInstanceView:view];    
}

- (void)removePluginInstanceViewsFor:(WebFrame*)webFrame 
{
    if (_private->pluginDatabase)
        [_private->pluginDatabase removePluginInstanceViewsFor:webFrame];    
}
#endif

- (BOOL)_isMIMETypeRegisteredAsPlugin:(NSString *)MIMEType
{
    if (![_private->preferences arePlugInsEnabled])
        return NO;

    if ([[WebPluginDatabase sharedDatabase] isMIMETypeRegistered:MIMEType])
        return YES;
        
#if !PLATFORM(IOS)
    if (_private->pluginDatabase && [_private->pluginDatabase isMIMETypeRegistered:MIMEType])
        return YES;
#endif
    
    return NO;
}

+ (BOOL)canShowMIMETypeAsHTML:(NSString *)MIMEType
{
#if PLATFORM(IOS)
    // FIXME: <rdar://problem/7961656> +[WebView canShowMIMETypeAsHTML:] regressed significantly in iOS 4.0
    // Fast path for the common case to avoid creating the MIME type registry.
    if ([MIMEType isEqualToString:@"text/html"])
        return YES;
#endif
    return [WebFrameView _canShowMIMETypeAsHTML:MIMEType];
}

+ (NSArray *)MIMETypesShownAsHTML
{
    NSMutableDictionary *viewTypes = [WebFrameView _viewTypesAllowImageTypeOmission:YES];
    NSEnumerator *enumerator = [viewTypes keyEnumerator];
    id key;
    NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
    
    while ((key = [enumerator nextObject])) {
        if ([viewTypes objectForKey:key] == [WebHTMLView class])
            [array addObject:key];
    }
    
    return array;
}

+ (void)setMIMETypesShownAsHTML:(NSArray *)MIMETypes
{
    NSDictionary *viewTypes = [[WebFrameView _viewTypesAllowImageTypeOmission:YES] copy];
    NSEnumerator *enumerator = [viewTypes keyEnumerator];
    id key;
    while ((key = [enumerator nextObject])) {
        if ([viewTypes objectForKey:key] == [WebHTMLView class])
            [WebView _unregisterViewClassAndRepresentationClassForMIMEType:key];
    }
    
    int i, count = [MIMETypes count];
    for (i = 0; i < count; i++) {
        [WebView registerViewClass:[WebHTMLView class] 
                representationClass:[WebHTMLRepresentation class] 
                forMIMEType:[MIMETypes objectAtIndex:i]];
    }
    [viewTypes release];
}

#if !PLATFORM(IOS)
+ (NSURL *)URLFromPasteboard:(NSPasteboard *)pasteboard
{
    return [pasteboard _web_bestURL];
}

+ (NSString *)URLTitleFromPasteboard:(NSPasteboard *)pasteboard
{
    return [pasteboard stringForType:WebURLNamePboardType];
}
#endif

+ (void)registerURLSchemeAsLocal:(NSString *)protocol
{
    SchemeRegistry::registerURLSchemeAsLocal(protocol);
}

- (id)_initWithArguments:(NSDictionary *) arguments
{
#if !PLATFORM(IOS)
    NSCoder *decoder = [arguments objectForKey:@"decoder"];
    if (decoder) {
        self = [self initWithCoder:decoder];
    } else {
#endif
        ASSERT([arguments objectForKey:@"frame"]);
        NSValue *frameValue = [arguments objectForKey:@"frame"];
        NSRect frame = (frameValue ? [frameValue rectValue] : NSZeroRect);
        NSString *frameName = [arguments objectForKey:@"frameName"];
        NSString *groupName = [arguments objectForKey:@"groupName"];
        self = [self initWithFrame:frame frameName:frameName groupName:groupName];
#if !PLATFORM(IOS)
    }
#endif

    return self;
}

#if !PLATFORM(IOS)
static bool clientNeedsWebViewInitThreadWorkaround()
{
    if (WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_WEBVIEW_INIT_THREAD_WORKAROUND))
        return false;

    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

    // Installer.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.installer"])
        return true;

    // Automator.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.Automator"])
        return true;

    // Automator Runner.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.AutomatorRunner"])
        return true;

    // Automator workflows.
    if ([bundleIdentifier _webkit_hasCaseInsensitivePrefix:@"com.apple.Automator."])
        return true;

    return false;
}

static bool needsWebViewInitThreadWorkaround()
{
    static bool isOldClient = clientNeedsWebViewInitThreadWorkaround();
    return isOldClient && !pthread_main_np();
}
#endif // !PLATFORM(IOS)

- (instancetype)initWithFrame:(NSRect)f
{
    return [self initWithFrame:f frameName:nil groupName:nil];
}

- (instancetype)initWithFrame:(NSRect)f frameName:(NSString *)frameName groupName:(NSString *)groupName
{
#if !PLATFORM(IOS)
    if (needsWebViewInitThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] initWithFrame:f frameName:frameName groupName:groupName];
#endif

    WebCoreThreadViolationCheckRoundTwo();
    return [self _initWithFrame:f frameName:frameName groupName:groupName];
}

#if !PLATFORM(IOS)
- (instancetype)initWithCoder:(NSCoder *)decoder
{
    if (needsWebViewInitThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] initWithCoder:decoder];

    WebCoreThreadViolationCheckRoundTwo();
    WebView *result = nil;

    @try {
        NSString *frameName;
        NSString *groupName;
        WebPreferences *preferences;
        BOOL useBackForwardList = NO;
        BOOL allowsUndo = YES;
        
        result = [super initWithCoder:decoder];
        result->_private = [[WebViewPrivate alloc] init];

        // We don't want any of the archived subviews. The subviews will always
        // be created in _commonInitializationFrameName:groupName:.
        [[result subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];

        if ([decoder allowsKeyedCoding]) {
            frameName = [decoder decodeObjectForKey:@"FrameName"];
            groupName = [decoder decodeObjectForKey:@"GroupName"];
            preferences = [decoder decodeObjectForKey:@"Preferences"];
            useBackForwardList = [decoder decodeBoolForKey:@"UseBackForwardList"];
            if ([decoder containsValueForKey:@"AllowsUndo"])
                allowsUndo = [decoder decodeBoolForKey:@"AllowsUndo"];
        } else {
            int version;
            [decoder decodeValueOfObjCType:@encode(int) at:&version];
            frameName = [decoder decodeObject];
            groupName = [decoder decodeObject];
            preferences = [decoder decodeObject];
            if (version > 1)
                [decoder decodeValuesOfObjCTypes:"c", &useBackForwardList];
            // The allowsUndo field is no longer written out in encodeWithCoder, but since there are
            // version 3 NIBs that have this field encoded, we still need to read it in.
            if (version == 3)
                [decoder decodeValuesOfObjCTypes:"c", &allowsUndo];
        }

        if (![frameName isKindOfClass:[NSString class]])
            frameName = nil;
        if (![groupName isKindOfClass:[NSString class]])
            groupName = nil;
        if (![preferences isKindOfClass:[WebPreferences class]])
            preferences = nil;

        LOG(Encoding, "FrameName = %@, GroupName = %@, useBackForwardList = %d\n", frameName, groupName, (int)useBackForwardList);
        [result _commonInitializationWithFrameName:frameName groupName:groupName];
        static_cast<BackForwardList*>([result page]->backForward().client())->setEnabled(useBackForwardList);
        result->_private->allowsUndo = allowsUndo;
        if (preferences)
            [result setPreferences:preferences];
    } @catch (NSException *localException) {
        result = nil;
        [self release];
    }

    return result;
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
    // Set asside the subviews before we archive. We don't want to archive any subviews.
    // The subviews will always be created in _commonInitializationFrameName:groupName:.
    id originalSubviews = _subviews;
    _subviews = nil;

    [super encodeWithCoder:encoder];

    // Restore the subviews we set aside.
    _subviews = originalSubviews;

    BOOL useBackForwardList = _private->page && static_cast<BackForwardList*>(_private->page->backForward().client())->enabled();
    if ([encoder allowsKeyedCoding]) {
        [encoder encodeObject:[[self mainFrame] name] forKey:@"FrameName"];
        [encoder encodeObject:[self groupName] forKey:@"GroupName"];
        [encoder encodeObject:[self preferences] forKey:@"Preferences"];
        [encoder encodeBool:useBackForwardList forKey:@"UseBackForwardList"];
        [encoder encodeBool:_private->allowsUndo forKey:@"AllowsUndo"];
    } else {
        int version = WebViewVersion;
        [encoder encodeValueOfObjCType:@encode(int) at:&version];
        [encoder encodeObject:[[self mainFrame] name]];
        [encoder encodeObject:[self groupName]];
        [encoder encodeObject:[self preferences]];
        [encoder encodeValuesOfObjCTypes:"c", &useBackForwardList];
        // DO NOT encode any new fields here, doing so will break older WebKit releases.
    }

    LOG(Encoding, "FrameName = %@, GroupName = %@, useBackForwardList = %d\n", [[self mainFrame] name], [self groupName], (int)useBackForwardList);
}
#endif // !PLATFORM(IOS)

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebView class], self))
        return;

    // Because the machinations of the view's shutdown may cause self to be added to
    // active autorelease pool, we capture any such releases here to ensure they are
    // carried out before we are dealloc'd.
    @autoreleasepool {

#if PLATFORM(IOS)
        if (_private)
            [_private->_geolocationProvider stopTrackingWebView:self];
#endif

        // call close to ensure we tear-down completely
        // this maintains our old behavior for existing applications
        [self close];

        if ([[self class] shouldIncludeInWebKitStatistics])
            --WebViewCount;

#if !PLATFORM(IOS)
        if ([self _needsFrameLoadDelegateRetainQuirk])
            [_private->frameLoadDelegate release];
#endif

        [_private release];
        // [super dealloc] can end up dispatching against _private (3466082)
        _private = nil;
    }

    [super dealloc];
}

- (void)finalize
{
    ASSERT(_private->closed);

    --WebViewCount;

    [super finalize];
}

- (void)close
{
    // _close existed first, and some clients might be calling or overriding it, so call through.
    [self _close];

#if PLATFORM(IOS)
    if (_private->layerFlushController) {
        _private->layerFlushController->invalidate();
        _private->layerFlushController = nullptr;
    }
#endif
}

- (void)setShouldCloseWithWindow:(BOOL)close
{
    _private->shouldCloseWithWindow = close;
}

- (BOOL)shouldCloseWithWindow
{
    return _private->shouldCloseWithWindow;
}

#if !PLATFORM(IOS)
// FIXME: Use AppKit constants for these when they are available.
static NSString * const windowDidChangeBackingPropertiesNotification = @"NSWindowDidChangeBackingPropertiesNotification";
static NSString * const backingPropertyOldScaleFactorKey = @"NSBackingPropertyOldScaleFactorKey"; 

- (void)addWindowObserversForWindow:(NSWindow *)window
{
    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowKeyStateChanged:)
            name:NSWindowDidBecomeKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowKeyStateChanged:)
            name:NSWindowDidResignKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillOrderOnScreen:)
            name:WKWindowWillOrderOnScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillOrderOffScreen:)
            name:WKWindowWillOrderOffScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidChangeBackingProperties:)
            name:windowDidChangeBackingPropertiesNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidChangeScreen:)
            name:NSWindowDidChangeScreenNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:) 
            name:NSWindowDidMiniaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:)
            name:NSWindowDidDeminiaturizeNotification object:window];
        [_private->windowVisibilityObserver startObserving:window];
    }
}

- (void)removeWindowObservers
{
    NSWindow *window = [self window];
    if (window) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidBecomeKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidResignKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:WKWindowWillOrderOnScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:WKWindowWillOrderOffScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:windowDidChangeBackingPropertiesNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidChangeScreenNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidMiniaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidDeminiaturizeNotification object:window];
        [_private->windowVisibilityObserver stopObserving:window];
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    // Don't do anything if the WebView isn't initialized.
    // This happens when decoding a WebView in a nib.
    // FIXME: What sets up the observer of NSWindowWillCloseNotification in this case?
    if (!_private)
        return;

    if ([self window] && [self window] != [self hostWindow])
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:[self window]];

    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:window];
         
        // Ensure that we will receive the events that WebHTMLView (at least) needs.
        // The following are expensive enough that we don't want to call them over
        // and over, so do them when we move into a window.
        [window setAcceptsMouseMovedEvents:YES];
        WKSetNSWindowShouldPostEventNotifications(window, YES);
    } else if (!_private->closed) {
        _private->page->setCanStartMedia(false);
        _private->page->setIsInWindow(false);
    }
        
    if (window != [self window]) {
        [self removeWindowObservers];
        [self addWindowObserversForWindow:window];
    }
}
#endif // !PLATFORM(IOS)

- (void)viewDidMoveToWindow
{
    // Don't do anything if we aren't initialized.  This happens
    // when decoding a WebView.  When WebViews are decoded their subviews
    // are created by initWithCoder: and so won't be normally
    // initialized.  The stub views are discarded by WebView.
    if (!_private || _private->closed)
        return;

    if ([self window]) {
        _private->page->setCanStartMedia(true);
        _private->page->setIsInWindow(true);

#if PLATFORM(IOS)
        WebPreferences *preferences = [self preferences];
        NSWindow *window = [self window];
        [window setTileBordersVisible:[preferences showDebugBorders]];
        [window setTilePaintCountsVisible:[preferences showRepaintCounter]];
        [window setAcceleratedDrawingEnabled:[preferences acceleratedDrawingEnabled]];
#endif
    }
#if PLATFORM(IOS)
    else
        [_private->fullscreenController requestHideAndExitFullscreen];
#endif

#if !PLATFORM(IOS)
    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
#endif

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    if (_private->immediateActionController) {
        NSImmediateActionGestureRecognizer *recognizer = [_private->immediateActionController immediateActionRecognizer];
        if ([self window]) {
            if (![[self gestureRecognizers] containsObject:recognizer])
                [self addGestureRecognizer:recognizer];
        } else
            [self removeGestureRecognizer:recognizer];
    }
#endif

    [self _updateActiveState];
    [self _updateVisibilityState];
}

#if !PLATFORM(IOS)
- (void)doWindowDidChangeScreen
{
    if (_private && _private->page)
        _private->page->chrome().windowScreenDidChange((PlatformDisplayID)[[[[[self window] screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue]);
}

- (void)_windowChangedKeyState
{
    [self _updateActiveState];
    [super _windowChangedKeyState];
}

- (void)windowKeyStateChanged:(NSNotification *)notification
{
    [self _updateActiveState];
}

- (void)viewDidHide
{
    [self _updateVisibilityState];
}

- (void)viewDidUnhide
{
    [self _updateVisibilityState];
}

- (void)_windowWillOrderOnScreen:(NSNotification *)notification
{
    if (![self shouldUpdateWhileOffscreen])
        [self setNeedsDisplay:YES];

    // Send a change screen to make sure the initial displayID is set
    [self doWindowDidChangeScreen];

    if (_private && _private->page) {
        _private->page->resumeScriptedAnimations();
        _private->page->setIsVisible(true);
    }
}

- (void)_windowDidChangeScreen:(NSNotification *)notification
{
    [self doWindowDidChangeScreen];
}

- (void)_windowWillOrderOffScreen:(NSNotification *)notification
{
    if (_private && _private->page) {
        _private->page->suspendScriptedAnimations();
        _private->page->setIsVisible(false);
    }
}

- (void)_windowVisibilityChanged:(NSNotification *)notification
{
    [self _updateVisibilityState];
}

- (void)_windowWillClose:(NSNotification *)notification
{
    if ([self shouldCloseWithWindow] && ([self window] == [self hostWindow] || ([self window] && ![self hostWindow]) || (![self window] && [self hostWindow])))
        [self close];
}

- (void)_windowDidChangeBackingProperties:(NSNotification *)notification
{
    CGFloat oldBackingScaleFactor = [[notification.userInfo objectForKey:backingPropertyOldScaleFactorKey] doubleValue]; 
    CGFloat newBackingScaleFactor = [self _deviceScaleFactor];
    if (oldBackingScaleFactor == newBackingScaleFactor) 
        return; 

    _private->page->setDeviceScaleFactor(newBackingScaleFactor);
}
#else
- (void)_wakWindowScreenScaleChanged:(NSNotification *)notification
{
    [self _updateScreenScaleFromWindow];
}

- (void)_wakWindowVisibilityChanged:(NSNotification *)notification
{
    if ([notification object] == [self window])
        [self _updateVisibilityState];
}

- (void)_updateScreenScaleFromWindow
{
    float scaleFactor = 1.0f;
    if (WAKWindow *window = [self window])
        scaleFactor = [window screenScale];
    else
        scaleFactor = WKGetScreenScaleFactor();

    _private->page->setDeviceScaleFactor(scaleFactor);
}
#endif // PLATFORM(IOS)

- (void)setPreferences:(WebPreferences *)prefs
{
    if (!prefs)
        prefs = [WebPreferences standardPreferences];

    if (_private->preferences == prefs)
        return;

    [prefs willAddToWebView];

    WebPreferences *oldPrefs = _private->preferences;

    [[NSNotificationCenter defaultCenter] removeObserver:self name:WebPreferencesChangedInternalNotification object:[self preferences]];
    [WebPreferences _removeReferenceForIdentifier:[oldPrefs identifier]];

    _private->preferences = [prefs retain];

    // After registering for the notification, post it so the WebCore settings update.
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesChangedNotification:)
        name:WebPreferencesChangedInternalNotification object:[self preferences]];
    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];

    [oldPrefs didRemoveFromWebView];
    [oldPrefs release];
}

- (WebPreferences *)preferences
{
    return _private->preferences;
}

- (void)setPreferencesIdentifier:(NSString *)anIdentifier
{
    if (!_private->closed && ![anIdentifier isEqual:[[self preferences] identifier]]) {
        WebPreferences *prefs = [[WebPreferences alloc] initWithIdentifier:anIdentifier];
        [self setPreferences:prefs];
        [prefs release];
    }
}

- (NSString *)preferencesIdentifier
{
    return [[self preferences] identifier];
}


- (void)setUIDelegate:delegate
{
    _private->UIDelegate = delegate;
#if PLATFORM(IOS)
    [_private->UIDelegateForwarder clearTarget];
#endif
    [_private->UIDelegateForwarder release];
    _private->UIDelegateForwarder = nil;
}

- (id)UIDelegate
{
    return _private->UIDelegate;
}

#if PLATFORM(IOS)
- (id)_resourceLoadDelegateForwarder
{
    if (_private->closing)
        return nil;

    if (!_private->resourceProgressDelegateForwarder)
        _private->resourceProgressDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:[self resourceLoadDelegate] defaultTarget:[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate]];
    return _private->resourceProgressDelegateForwarder;
}
#endif

- (void)setResourceLoadDelegate: delegate
{
#if PLATFORM(IOS)
    [_private->resourceProgressDelegateForwarder clearTarget];
    [_private->resourceProgressDelegateForwarder release];
    _private->resourceProgressDelegateForwarder = nil;
#endif
    _private->resourceProgressDelegate = delegate;
    [self _cacheResourceLoadDelegateImplementations];
}

- (id)resourceLoadDelegate
{
    return _private->resourceProgressDelegate;
}

- (void)setDownloadDelegate: delegate
{
    _private->downloadDelegate = delegate;
}


- (id)downloadDelegate
{
    return _private->downloadDelegate;
}

- (void)setPolicyDelegate:delegate
{
    _private->policyDelegate = delegate;
#if PLATFORM(IOS)
    [_private->policyDelegateForwarder clearTarget];
#endif
    [_private->policyDelegateForwarder release];
    _private->policyDelegateForwarder = nil;
}

- (id)policyDelegate
{
    return _private->policyDelegate;
}

#if PLATFORM(IOS)
- (id)_frameLoadDelegateForwarder
{
    if (_private->closing)
        return nil;

    if (!_private->frameLoadDelegateForwarder)
        _private->frameLoadDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:[self frameLoadDelegate] defaultTarget:[WebDefaultFrameLoadDelegate sharedFrameLoadDelegate]];
    return _private->frameLoadDelegateForwarder;
}
#endif

- (void)setFrameLoadDelegate:delegate
{
    // <rdar://problem/6950660> - Due to some subtle WebKit changes - presumably to delegate callback behavior - we've
    // unconvered a latent bug in at least one WebKit app where the delegate wasn't properly retained by the app and
    // was dealloc'ed before being cleared.
    // This is an effort to keep such apps working for now.
#if !PLATFORM(IOS)
    if ([self _needsFrameLoadDelegateRetainQuirk]) {
        [delegate retain];
        [_private->frameLoadDelegate release];
    }
#else
    [_private->frameLoadDelegateForwarder clearTarget];
    [_private->frameLoadDelegateForwarder release];
    _private->frameLoadDelegateForwarder = nil;
#endif

    _private->frameLoadDelegate = delegate;
    [self _cacheFrameLoadDelegateImplementations];

#if ENABLE(ICONDATABASE)
    // If this delegate wants callbacks for icons, fire up the icon database.
    if (_private->frameLoadDelegateImplementations.didReceiveIconForFrameFunc)
        [WebIconDatabase sharedIconDatabase];
#endif
}

- (id)frameLoadDelegate
{
    return _private->frameLoadDelegate;
}

- (WebFrame *)mainFrame
{
    // This can be called in initialization, before _private has been set up (3465613)
    if (!_private || !_private->page)
        return nil;
    return kit(&_private->page->mainFrame());
}

- (WebFrame *)selectedFrame
{
    // If the first responder is a view in our tree, we get the frame containing the first responder.
    // This is faster than searching the frame hierarchy, and will give us a result even in the case
    // where the focused frame doesn't actually contain a selection.
    WebFrame *focusedFrame = [self _focusedFrame];
    if (focusedFrame)
        return focusedFrame;
    
    // If the first responder is outside of our view tree, we search for a frame containing a selection.
    // There should be at most only one of these.
    return [[self mainFrame] _findFrameWithSelection];
}

- (WebBackForwardList *)backForwardList
{
    if (!_private->page)
        return nil;
    BackForwardList* list = static_cast<BackForwardList*>(_private->page->backForward().client());
    if (!list->enabled())
        return nil;
    return kit(list);
}

- (void)setMaintainsBackForwardList:(BOOL)flag
{
    if (!_private->page)
        return;
    static_cast<BackForwardList*>(_private->page->backForward().client())->setEnabled(flag);
}

- (BOOL)goBack
{
    if (!_private->page)
        return NO;
    
#if PLATFORM(IOS)
    if (WebThreadIsCurrent() || !WebThreadIsEnabled())
#endif
    return _private->page->backForward().goBack();
#if PLATFORM(IOS)
    WebThreadRun(^{
        _private->page->backForward().goBack();
    });
    // FIXME: <rdar://problem/9157572> -[WebView goBack] and -goForward always return YES when called from the main thread
    return YES;
#endif
}

- (BOOL)goForward
{
    if (!_private->page)
        return NO;

#if PLATFORM(IOS)
    if (WebThreadIsCurrent() || !WebThreadIsEnabled())
#endif
    return _private->page->backForward().goForward();
#if PLATFORM(IOS)
    WebThreadRun(^{
        _private->page->backForward().goForward();
    });
    // FIXME: <rdar://problem/9157572> -[WebView goBack] and -goForward always return YES when called from the main thread
    return YES;
#endif
}

- (BOOL)goToBackForwardItem:(WebHistoryItem *)item
{
    if (!_private->page)
        return NO;

    ASSERT(item);
    _private->page->goToItem(*core(item), FrameLoadType::IndexedBackForward);
    return YES;
}

- (void)setTextSizeMultiplier:(float)m
{
    [self _setZoomMultiplier:m isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (float)textSizeMultiplier
{
    return [self _realZoomMultiplierIsTextOnly] ? _private->zoomMultiplier : 1.0f;
}

- (void)_setZoomMultiplier:(float)multiplier isTextOnly:(BOOL)isTextOnly
{
    // NOTE: This has no visible effect when viewing a PDF (see <rdar://problem/4737380>)
    _private->zoomMultiplier = multiplier;
    _private->zoomsTextOnly = isTextOnly;

    // FIXME: It might be nice to rework this code so that _private->zoomMultiplier doesn't exist
    // and instead the zoom factors stored in Frame are used.
    Frame* coreFrame = [self _mainCoreFrame];
    if (coreFrame) {
        if (_private->zoomsTextOnly)
            coreFrame->setPageAndTextZoomFactors(1, multiplier);
        else
            coreFrame->setPageAndTextZoomFactors(multiplier, 1);
    }
}

- (float)_zoomMultiplier:(BOOL)isTextOnly
{
    if (isTextOnly != [self _realZoomMultiplierIsTextOnly])
        return 1.0f;
    return _private->zoomMultiplier;
}

- (float)_realZoomMultiplier
{
    return _private->zoomMultiplier;
}

- (BOOL)_realZoomMultiplierIsTextOnly
{
    if (!_private->page)
        return NO;
    
    return _private->zoomsTextOnly;
}

#define MinimumZoomMultiplier       0.5f
#define MaximumZoomMultiplier       3.0f
#define ZoomMultiplierRatio         1.2f

- (BOOL)_canZoomOut:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canZoomOut];
    }
    return [self _zoomMultiplier:isTextOnly] / ZoomMultiplierRatio > MinimumZoomMultiplier;
}


- (BOOL)_canZoomIn:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canZoomIn];
    }
    return [self _zoomMultiplier:isTextOnly] * ZoomMultiplierRatio < MaximumZoomMultiplier;
}

- (IBAction)_zoomOut:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _zoomOut:sender];
    }
    float newScale = [self _zoomMultiplier:isTextOnly] / ZoomMultiplierRatio;
    if (newScale > MinimumZoomMultiplier)
        [self _setZoomMultiplier:newScale isTextOnly:isTextOnly];
}

- (IBAction)_zoomIn:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _zoomIn:sender];
    }
    float newScale = [self _zoomMultiplier:isTextOnly] * ZoomMultiplierRatio;
    if (newScale < MaximumZoomMultiplier)
        [self _setZoomMultiplier:newScale isTextOnly:isTextOnly];
}

- (BOOL)_canResetZoom:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canResetZoom];
    }
    return [self _zoomMultiplier:isTextOnly] != 1.0f;
}

- (IBAction)_resetZoom:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _resetZoom:sender];
    }
    if ([self _zoomMultiplier:isTextOnly] != 1.0f)
        [self _setZoomMultiplier:1.0f isTextOnly:isTextOnly];
}

- (void)setApplicationNameForUserAgent:(NSString *)applicationName
{
    NSString *name = [applicationName copy];
    [_private->applicationNameForUserAgent release];
    _private->applicationNameForUserAgent = name;
    if (!_private->userAgentOverridden)
        _private->userAgent = String();
}

- (NSString *)applicationNameForUserAgent
{
    return [[_private->applicationNameForUserAgent retain] autorelease];
}

- (void)setCustomUserAgent:(NSString *)userAgentString
{
    _private->userAgent = userAgentString;
    _private->userAgentOverridden = userAgentString != nil;
}

- (NSString *)customUserAgent
{
    if (!_private->userAgentOverridden)
        return nil;
    return _private->userAgent;
}

- (void)setMediaStyle:(NSString *)mediaStyle
{
    if (_private->mediaStyle != mediaStyle) {
        [_private->mediaStyle release];
        _private->mediaStyle = [mediaStyle copy];
    }
}

- (NSString *)mediaStyle
{
    return _private->mediaStyle;
}

- (BOOL)supportsTextEncoding
{
    id documentView = [[[self mainFrame] frameView] documentView];
    return [documentView conformsToProtocol:@protocol(WebDocumentText)]
        && [documentView supportsTextEncoding];
}

- (void)setCustomTextEncodingName:(NSString *)encoding
{
    NSString *oldEncoding = [self customTextEncodingName];
    if (encoding == oldEncoding || [encoding isEqualToString:oldEncoding])
        return;
    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->loader().reloadWithOverrideEncoding(encoding);
}

- (NSString *)_mainFrameOverrideEncoding
{
    WebDataSource *dataSource = [[self mainFrame] provisionalDataSource];
    if (dataSource == nil)
        dataSource = [[self mainFrame] _dataSource];
    if (dataSource == nil)
        return nil;
    return nsStringNilIfEmpty([dataSource _documentLoader]->overrideEncoding());
}

- (NSString *)customTextEncodingName
{
    return [self _mainFrameOverrideEncoding];
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)script
{
#if !PLATFORM(IOS)
    // Return statements are only valid in a function but some applications pass in scripts
    // prefixed with return (<rdar://problems/5103720&4616860>) since older WebKit versions
    // silently ignored the return. If the application is linked against an earlier version
    // of WebKit we will strip the return so the script wont fail.
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_JAVASCRIPT_RETURN_QUIRK)) {
        NSRange returnStringRange = [script rangeOfString:@"return "];
        if (returnStringRange.length && !returnStringRange.location)
            script = [script substringFromIndex:returnStringRange.location + returnStringRange.length];
    }
#endif

    NSString *result = [[self mainFrame] _stringByEvaluatingJavaScriptFromString:script];
    // The only way stringByEvaluatingJavaScriptFromString can return nil is if the frame was removed by the script
    // Since there's no way to get rid of the main frame, result will never ever be nil here.
    ASSERT(result);

    return result;
}

- (WebScriptObject *)windowScriptObject
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    return coreFrame->script().windowScriptObject();
}

- (String)_userAgentString
{
    if (_private->userAgent.isNull())
        _private->userAgent = [[self class] _standardUserAgentWithApplicationName:_private->applicationNameForUserAgent];

    return _private->userAgent;
}

// Get the appropriate user-agent string for a particular URL.
- (NSString *)userAgentForURL:(NSURL *)url
{
    return [self _userAgentString];
}

- (void)setHostWindow:(NSWindow *)hostWindow
{
    if (_private->closed && hostWindow)
        return;
    if (hostWindow == _private->hostWindow)
        return;

    Frame* coreFrame = [self _mainCoreFrame];
#if !PLATFORM(IOS)
    for (Frame* frame = coreFrame; frame; frame = frame->tree().traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewWillMoveToHostWindow:hostWindow];
    if (_private->hostWindow && [self window] != _private->hostWindow)
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:_private->hostWindow];
    if (hostWindow)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:hostWindow];
#endif
    [_private->hostWindow release];
    _private->hostWindow = [hostWindow retain];
    for (Frame* frame = coreFrame; frame; frame = frame->tree().traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewDidMoveToHostWindow];
#if !PLATFORM(IOS)
    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
#endif
}

- (NSWindow *)hostWindow
{
    // -[WebView hostWindow] can sometimes be called from the WebView's [super dealloc] method
    // so we check here to make sure it's not null.
    if (!_private)
        return nil;
    
    return _private->hostWindow;
}

- (NSView <WebDocumentView> *)documentViewAtWindowPoint:(NSPoint)point
{
    return [[self _frameViewAtWindowPoint:point] documentView];
}

- (NSDictionary *)_elementAtWindowPoint:(NSPoint)windowPoint
{
    WebFrameView *frameView = [self _frameViewAtWindowPoint:windowPoint];
    if (!frameView)
        return nil;
    NSView <WebDocumentView> *documentView = [frameView documentView];
    if ([documentView conformsToProtocol:@protocol(WebDocumentElement)]) {
        NSPoint point = [documentView convertPoint:windowPoint fromView:nil];
        return [(NSView <WebDocumentElement> *)documentView elementAtPoint:point];
    }
    return [NSDictionary dictionaryWithObject:[frameView webFrame] forKey:WebElementFrameKey];
}

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    return [self _elementAtWindowPoint:[self convertPoint:point toView:nil]];
}

#if ENABLE(DRAG_SUPPORT)
// The following 2 internal NSView methods are called on the drag destination to make scrolling while dragging work.
// Scrolling while dragging will only work if the drag destination is in a scroll view. The WebView is the drag destination. 
// When dragging to a WebView, the document subview should scroll, but it doesn't because it is not the drag destination. 
// Forward these calls to the document subview to make its scroll view scroll.
- (void)_autoscrollForDraggingInfo:(id)draggingInfo timeDelta:(NSTimeInterval)repeatDelta
{
    NSView <WebDocumentView> *documentView = [self documentViewAtWindowPoint:[draggingInfo draggingLocation]];
    [documentView _autoscrollForDraggingInfo:draggingInfo timeDelta:repeatDelta];
}

- (BOOL)_shouldAutoscrollForDraggingInfo:(id)draggingInfo
{
    NSView <WebDocumentView> *documentView = [self documentViewAtWindowPoint:[draggingInfo draggingLocation]];
    return [documentView _shouldAutoscrollForDraggingInfo:draggingInfo];
}

- (DragApplicationFlags)applicationFlags:(id <NSDraggingInfo>)draggingInfo
{
    uint32_t flags = 0;
    if ([NSApp modalWindow])
        flags = DragApplicationIsModal;
    if ([[self window] attachedSheet])
        flags |= DragApplicationHasAttachedSheet;
    if ([draggingInfo draggingSource] == self)
        flags |= DragApplicationIsSource;
    if ([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        flags |= DragApplicationIsCopyKeyDown;
    return static_cast<DragApplicationFlags>(flags);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return core(self)->dragController().dragEntered(dragData);
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)draggingInfo
{
    Page* page = core(self);
    if (!page)
        return NSDragOperationNone;

    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return page->dragController().dragUpdated(dragData);
}

- (void)draggingExited:(id <NSDraggingInfo>)draggingInfo
{
    Page* page = core(self);
    if (!page)
        return;

    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    page->dragController().dragExited(dragData);
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return core(self)->dragController().performDragOperation(dragData);
}

- (NSView *)_hitTest:(NSPoint *)point dragTypes:(NSSet *)types
{
    NSView *hitView = [super _hitTest:point dragTypes:types];
    if (!hitView && [[self superview] mouse:*point inRect:[self frame]])
        return self;
    return hitView;
}
#endif

- (BOOL)acceptsFirstResponder
{
    return [[[self mainFrame] frameView] acceptsFirstResponder];
}

- (BOOL)becomeFirstResponder
{
    if (_private->becomingFirstResponder) {
        // Fix for unrepro infinite recursion reported in Radar 4448181. If we hit this assert on
        // a debug build, we should figure out what causes the problem and do a better fix.
        ASSERT_NOT_REACHED();
        return NO;
    }
    
    // This works together with setNextKeyView to splice the WebView into
    // the key loop similar to the way NSScrollView does this. Note that
    // WebFrameView has very similar code.
#if !PLATFORM(IOS)
    NSWindow *window = [self window];
#endif
    WebFrameView *mainFrameView = [[self mainFrame] frameView];

#if !PLATFORM(IOS)
    NSResponder *previousFirstResponder = [[self window] _oldFirstResponderBeforeBecoming];
    BOOL fromOutside = ![previousFirstResponder isKindOfClass:[NSView class]] || (![(NSView *)previousFirstResponder isDescendantOf:self] && previousFirstResponder != self);

    if ([window keyViewSelectionDirection] == NSSelectingPrevious) {
        NSView *previousValidKeyView = [self previousValidKeyView];
        if (previousValidKeyView != self && previousValidKeyView != mainFrameView) {
            _private->becomingFirstResponder = YES;
            _private->becomingFirstResponderFromOutside = fromOutside;
            [window makeFirstResponder:previousValidKeyView];
            _private->becomingFirstResponderFromOutside = NO;
            _private->becomingFirstResponder = NO;
            return YES;
        }
        return NO;
    }
#endif

    if ([mainFrameView acceptsFirstResponder]) {
#if !PLATFORM(IOS)
        _private->becomingFirstResponder = YES;
        _private->becomingFirstResponderFromOutside = fromOutside;
        [window makeFirstResponder:mainFrameView];
        _private->becomingFirstResponderFromOutside = NO;
        _private->becomingFirstResponder = NO;
#endif
        return YES;
    } 

    return NO;
}

- (NSView *)_webcore_effectiveFirstResponder
{
    if (WebFrameView *frameView = [[self mainFrame] frameView])
        return [frameView _webcore_effectiveFirstResponder];

    return [super _webcore_effectiveFirstResponder];
}

- (void)setNextKeyView:(NSView *)view
{
    // This works together with becomeFirstResponder to splice the WebView into
    // the key loop similar to the way NSScrollView does this. Note that
    // WebFrameView has similar code.
    if (WebFrameView *mainFrameView = [[self mainFrame] frameView]) {
        [mainFrameView setNextKeyView:view];
        return;
    }

    [super setNextKeyView:view];
}

static WebFrame *incrementFrame(WebFrame *frame, WebFindOptions options = 0)
{
    Frame* coreFrame = core(frame);
    return kit((options & WebFindOptionsBackwards)
        ? coreFrame->tree().traversePreviousWithWrap(options & WebFindOptionsWrapAround)
        : coreFrame->tree().traverseNextWithWrap(options & WebFindOptionsWrapAround));
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag
{
    return [self searchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag startInSelection:NO];
}

+ (void)registerViewClass:(Class)viewClass representationClass:(Class)representationClass forMIMEType:(NSString *)MIMEType
{
    [[WebFrameView _viewTypesAllowImageTypeOmission:YES] setObject:viewClass forKey:MIMEType];
    [[WebDataSource _repTypesAllowImageTypeOmission:YES] setObject:representationClass forKey:MIMEType];
    
    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    if ([viewClass class] == [WebHTMLView class])
        MIMETypeRegistry::getSupportedNonImageMIMETypes().add(MIMEType);
}

- (void)setGroupName:(NSString *)groupName
{
    if (_private->group)
        _private->group->removeWebView(self);

    _private->group = WebViewGroup::getOrCreate(groupName, _private->preferences._localStorageDatabasePath);
    _private->group->addWebView(self);

    if (!_private->page)
        return;

    _private->page->setUserContentController(&_private->group->userContentController());
    _private->page->setVisitedLinkStore(_private->group->visitedLinkStore());
    _private->page->setGroupName(groupName);
}

- (NSString *)groupName
{
    if (!_private->page)
        return nil;
    return _private->page->groupName();
}

- (double)estimatedProgress
{
    if (!_private->page)
        return 0.0;
    return _private->page->progress().estimatedProgress();
}

#if !PLATFORM(IOS)
- (NSArray *)pasteboardTypesForSelection
{
    NSView <WebDocumentView> *documentView = [[[self _selectedOrMainFrame] frameView] documentView];
    if ([documentView conformsToProtocol:@protocol(WebDocumentSelection)]) {
        return [(NSView <WebDocumentSelection> *)documentView pasteboardTypesForSelection];
    }
    return [NSArray array];
}

- (void)writeSelectionWithPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    WebFrame *frame = [self _selectedOrMainFrame];
    if (frame && [frame _hasSelection]) {
        NSView <WebDocumentView> *documentView = [[frame frameView] documentView];
        if ([documentView conformsToProtocol:@protocol(WebDocumentSelection)])
            [(NSView <WebDocumentSelection> *)documentView writeSelectionWithPasteboardTypes:types toPasteboard:pasteboard];
    }
}

- (NSArray *)pasteboardTypesForElement:(NSDictionary *)element
{
    if ([element objectForKey:WebElementImageURLKey] != nil) {
        return [NSPasteboard _web_writableTypesForImageIncludingArchive:([element objectForKey:WebElementDOMNodeKey] != nil)];
    } else if ([element objectForKey:WebElementLinkURLKey] != nil) {
        return [NSPasteboard _web_writableTypesForURL];
    } else if ([[element objectForKey:WebElementIsSelectedKey] boolValue]) {
        return [self pasteboardTypesForSelection];
    }
    return [NSArray array];
}

- (void)writeElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    if ([element objectForKey:WebElementImageURLKey] != nil) {
        [self _writeImageForElement:element withPasteboardTypes:types toPasteboard:pasteboard];
    } else if ([element objectForKey:WebElementLinkURLKey] != nil) {
        [self _writeLinkElement:element withPasteboardTypes:types toPasteboard:pasteboard];
    } else if ([[element objectForKey:WebElementIsSelectedKey] boolValue]) {
        [self writeSelectionWithPasteboardTypes:types toPasteboard:pasteboard];
    }
}

- (void)moveDragCaretToPoint:(NSPoint)point
{
#if ENABLE(DRAG_SUPPORT)
    if (Page* page = core(self))
        page->dragController().placeDragCaret(IntPoint([self convertPoint:point toView:nil]));
#endif
}

- (void)removeDragCaret
{
#if ENABLE(DRAG_SUPPORT)
    if (Page* page = core(self))
        page->dragController().dragEnded();
#endif
}
#endif // !PLATFORM(IOS)

- (void)setMainFrameURL:(NSString *)URLString
{
    NSURL *url;
    if ([URLString hasPrefix:@"/"])
        url = [NSURL fileURLWithPath:URLString];
    else
        url = [NSURL _web_URLWithDataAsString:URLString];

    [[self mainFrame] loadRequest:[NSURLRequest requestWithURL:url]];
}

- (NSString *)mainFrameURL
{
    WebDataSource *ds;
    ds = [[self mainFrame] provisionalDataSource];
    if (!ds)
        ds = [[self mainFrame] _dataSource];
    return [[[ds request] URL] _web_originalDataAsString];
}

- (BOOL)isLoading
{
    LOG (Bindings, "isLoading = %d", (int)[self _isLoading]);
    return [self _isLoading];
}

- (NSString *)mainFrameTitle
{
    NSString *mainFrameTitle = [[[self mainFrame] _dataSource] pageTitle];
    return (mainFrameTitle != nil) ? mainFrameTitle : (NSString *)@"";
}

#if !PLATFORM(IOS)
- (NSImage *)mainFrameIcon
{
    return [[WebIconDatabase sharedIconDatabase] iconForURL:[[[[self mainFrame] _dataSource] _URL] _web_originalDataAsString] withSize:WebIconSmallSize];
}
#else
- (NSURL *)mainFrameIconURL
{
    WebFrame *mainFrame = [self mainFrame];
    Frame *coreMainFrame = core(mainFrame);
    if (!coreMainFrame)
        return nil;

    NSURL *url = (NSURL *)coreMainFrame->loader().icon().url();
    return url;
}
#endif

- (DOMDocument *)mainFrameDocument
{
    // only return the actual value if the state we're in gives NSTreeController
    // enough time to release its observers on the old model
    if (_private->mainFrameDocumentReady)
        return [[self mainFrame] DOMDocument];
    return nil;
}

- (void)setDrawsBackground:(BOOL)drawsBackground
{
    if (_private->drawsBackground == drawsBackground)
        return;
    _private->drawsBackground = drawsBackground;
    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (BOOL)drawsBackground
{
    // This method can be called beneath -[NSView dealloc] after we have cleared _private,
    // indirectly via -[WebFrameView viewDidMoveToWindow].
    return !_private || _private->drawsBackground;
}

- (void)setShouldUpdateWhileOffscreen:(BOOL)updateWhileOffscreen
{
    if (_private->shouldUpdateWhileOffscreen == updateWhileOffscreen)
        return;
    _private->shouldUpdateWhileOffscreen = updateWhileOffscreen;
    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (BOOL)shouldUpdateWhileOffscreen
{
    return _private->shouldUpdateWhileOffscreen;
}

- (void)setCurrentNodeHighlight:(WebNodeHighlight *)nodeHighlight
{
    id old = _private->currentNodeHighlight;
    _private->currentNodeHighlight = [nodeHighlight retain];
    [old release];
}

- (WebNodeHighlight *)currentNodeHighlight
{
    return _private->currentNodeHighlight;
}

- (NSView *)previousValidKeyView
{
    NSView *result = [super previousValidKeyView];

    // Work around AppKit bug 6905484. If the result is a view that's inside this one, it's
    // possible it is the wrong answer, because the fact that it's a descendant causes the
    // code that implements key view redirection to fail; this means we won't redirect to
    // the toolbar, for example, when we hit the edge of a window. Since the bug is specific
    // to cases where the receiver of previousValidKeyView is an ancestor of the last valid
    // key view in the loop, we can sidestep it by walking along previous key views until
    // we find one that is not a superview, then using that to call previousValidKeyView.

    if (![result isDescendantOf:self])
        return result;

    // Use a visited set so we don't loop indefinitely when walking crazy key loops.
    // AppKit uses such sets internally and we want our loop to be as robust as its loops.
    RetainPtr<CFMutableSetRef> visitedViews = adoptCF(CFSetCreateMutable(0, 0, 0));
    CFSetAddValue(visitedViews.get(), result);

    NSView *previousView = self;
    do {
        CFSetAddValue(visitedViews.get(), previousView);
        previousView = [previousView previousKeyView];
        if (!previousView || CFSetGetValue(visitedViews.get(), previousView))
            return result;
    } while ([result isDescendantOf:previousView]);
    return [previousView previousValidKeyView];
}

@end

@implementation WebView (WebIBActions)

- (IBAction)takeStringURLFrom: sender
{
    NSString *URLString = [sender stringValue];
    
    [[self mainFrame] loadRequest: [NSURLRequest requestWithURL: [NSURL _web_URLWithDataAsString: URLString]]];
}

- (BOOL)canGoBack
{
#if PLATFORM(IOS)
    WebThreadLock();
    if (!_private->page)
#else
    if (!_private->page || _private->page->defersLoading())
#endif
        return NO;

    return _private->page->backForward().canGoBackOrForward(-1);
}

- (BOOL)canGoForward
{
#if PLATFORM(IOS)
    WebThreadLock();
    if (!_private->page)
#else
    if (!_private->page || _private->page->defersLoading())
#endif
        return NO;

    return !!_private->page->backForward().canGoBackOrForward(1);
}

- (IBAction)goBack:(id)sender
{
    [self goBack];
}

- (IBAction)goForward:(id)sender
{
    [self goForward];
}

- (IBAction)stopLoading:(id)sender
{
#if PLATFORM(IOS)
    if (WebThreadNotCurrent()) {
        _private->isStopping = true;
        WebThreadSetShouldYield();
    }
    WebThreadRun(^{
        _private->isStopping = false;
#endif
    [[self mainFrame] stopLoading];
#if PLATFORM(IOS)
    });
#endif
}

#if PLATFORM(IOS)
- (void)stopLoadingAndClear
{
    if (WebThreadNotCurrent() && !WebThreadIsLocked()) {
        _private->isStopping = true;
        WebThreadSetShouldYield();
    }
    WebThreadRun(^{
        _private->isStopping = false;

        WebFrame *frame = [self mainFrame];
        [frame stopLoading];
        core(frame)->document()->loader()->writer().end(); // End to finish parsing and display immediately

        WebFrameView *mainFrameView = [frame frameView];
        float scale = [[mainFrameView documentView] scale];
        WebPlainWhiteView *plainWhiteView = [[WebPlainWhiteView alloc] initWithFrame:NSZeroRect];
        [plainWhiteView setScale:scale];
        [plainWhiteView setFrame:[mainFrameView bounds]];
        [mainFrameView _setDocumentView:plainWhiteView];
        [plainWhiteView setNeedsDisplay:YES];
        [plainWhiteView release];
    });
}
#endif

- (IBAction)reload:(id)sender
{
#if PLATFORM(IOS)
    WebThreadRun(^{
#endif            
    [[self mainFrame] reload];
#if PLATFORM(IOS)
    });
#endif            
}

- (IBAction)reloadFromOrigin:(id)sender
{
    [[self mainFrame] reloadFromOrigin];
}

// FIXME: This code should move into WebCore so that it is not duplicated in each WebKit.
// (This includes canMakeTextSmaller/Larger, makeTextSmaller/Larger, and canMakeTextStandardSize/makeTextStandardSize)
- (BOOL)canMakeTextSmaller
{
    return [self _canZoomOut:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextSmaller:(id)sender
{
    return [self _zoomOut:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (BOOL)canMakeTextLarger
{
    return [self _canZoomIn:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextLarger:(id)sender
{
    return [self _zoomIn:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (BOOL)canMakeTextStandardSize
{
    return [self _canResetZoom:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextStandardSize:(id)sender
{
   return [self _resetZoom:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

#if !PLATFORM(IOS)
- (IBAction)toggleSmartInsertDelete:(id)sender
{
    [self setSmartInsertDeleteEnabled:![self smartInsertDeleteEnabled]];
}

- (IBAction)toggleContinuousSpellChecking:(id)sender
{
    [self setContinuousSpellCheckingEnabled:![self isContinuousSpellCheckingEnabled]];
}

- (BOOL)_responderValidateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    id responder = [self _responderForResponderOperations];
    if (responder != self && [responder respondsToSelector:[item action]]) {
        if ([responder respondsToSelector:@selector(validateUserInterfaceItemWithoutDelegate:)])
            return [responder validateUserInterfaceItemWithoutDelegate:item];
        if ([responder respondsToSelector:@selector(validateUserInterfaceItem:)])
            return [responder validateUserInterfaceItem:item];
        return YES;
    }
    return NO;
}

#define VALIDATE(name) \
    else if (action == @selector(name:)) { return [self _responderValidateUserInterfaceItem:item]; }

- (BOOL)validateUserInterfaceItemWithoutDelegate:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];

    if (action == @selector(goBack:)) {
        return [self canGoBack];
    } else if (action == @selector(goForward:)) {
        return [self canGoForward];
    } else if (action == @selector(makeTextLarger:)) {
        return [self canMakeTextLarger];
    } else if (action == @selector(makeTextSmaller:)) {
        return [self canMakeTextSmaller];
    } else if (action == @selector(makeTextStandardSize:)) {
        return [self canMakeTextStandardSize];
    } else if (action == @selector(reload:)) {
        return [[self mainFrame] _dataSource] != nil;
    } else if (action == @selector(stopLoading:)) {
        return [self _isLoading];
    } else if (action == @selector(toggleContinuousSpellChecking:)) {
        BOOL checkMark = NO;
        BOOL retVal = NO;
        if ([self _continuousCheckingAllowed]) {
            checkMark = [self isContinuousSpellCheckingEnabled];
            retVal = YES;
        }
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return retVal;
    } else if (action == @selector(toggleSmartInsertDelete:)) {
        BOOL checkMark = [self smartInsertDeleteEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleGrammarChecking:)) {
        BOOL checkMark = [self isGrammarCheckingEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticQuoteSubstitution:)) {
        BOOL checkMark = [self isAutomaticQuoteSubstitutionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticLinkDetection:)) {
        BOOL checkMark = [self isAutomaticLinkDetectionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticDashSubstitution:)) {
        BOOL checkMark = [self isAutomaticDashSubstitutionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticTextReplacement:)) {
        BOOL checkMark = [self isAutomaticTextReplacementEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticSpellingCorrection:)) {
        BOOL checkMark = [self isAutomaticSpellingCorrectionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    }
    FOR_EACH_RESPONDER_SELECTOR(VALIDATE)

    return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    BOOL result = [self validateUserInterfaceItemWithoutDelegate:item];
    return CallUIDelegateReturningBoolean(result, self, @selector(webView:validateUserInterfaceItem:defaultValidation:), item, result);
}
#endif // !PLATFORM(IOS)

@end

@implementation WebView (WebPendingPublic)

#if !PLATFORM(IOS)
- (void)scheduleInRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode
{
#if USE(CFNETWORK)
    CFRunLoopRef schedulePairRunLoop = [runLoop getCFRunLoop];
#else
    NSRunLoop *schedulePairRunLoop = runLoop;
#endif
    if (runLoop && mode)
        core(self)->addSchedulePair(SchedulePair::create(schedulePairRunLoop, (CFStringRef)mode));
}

- (void)unscheduleFromRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode
{
#if USE(CFNETWORK)
    CFRunLoopRef schedulePairRunLoop = [runLoop getCFRunLoop];
#else
    NSRunLoop *schedulePairRunLoop = runLoop;
#endif
    if (runLoop && mode)
        core(self)->removeSchedulePair(SchedulePair::create(schedulePairRunLoop, (CFStringRef)mode));
}
#endif

static BOOL findString(NSView <WebDocumentSearching> *searchView, NSString *string, WebFindOptions options)
{
    if ([searchView conformsToProtocol:@protocol(WebDocumentOptionsSearching)])
        return [(NSView <WebDocumentOptionsSearching> *)searchView _findString:string options:options];
    if ([searchView conformsToProtocol:@protocol(WebDocumentIncrementalSearching)])
        return [(NSView <WebDocumentIncrementalSearching> *)searchView searchFor:string direction:!(options & WebFindOptionsBackwards) caseSensitive:!(options & WebFindOptionsCaseInsensitive) wrap:!!(options & WebFindOptionsWrapAround) startInSelection:!!(options & WebFindOptionsStartInSelection)];
    return [searchView searchFor:string direction:!(options & WebFindOptionsBackwards) caseSensitive:!(options & WebFindOptionsCaseInsensitive) wrap:!!(options & WebFindOptionsWrapAround)];
}

- (BOOL)findString:(NSString *)string options:(WebFindOptions)options
{
    if (_private->closed)
        return NO;
    
    // Get the frame holding the selection, or start with the main frame
    WebFrame *startFrame = [self _selectedOrMainFrame];
    
    // Search the first frame, then all the other frames, in order
    NSView <WebDocumentSearching> *startSearchView = nil;
    WebFrame *frame = startFrame;
    do {
        WebFrame *nextFrame = incrementFrame(frame, options);
        
        BOOL onlyOneFrame = (frame == nextFrame);
        ASSERT(!onlyOneFrame || frame == startFrame);
        
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebDocumentSearching)]) {
            NSView <WebDocumentSearching> *searchView = (NSView <WebDocumentSearching> *)view;
            
            if (frame == startFrame)
                startSearchView = searchView;
            
            // In some cases we have to search some content twice; see comment later in this method.
            // We can avoid ever doing this in the common one-frame case by passing the wrap option through 
            // here, and then bailing out before we get to the code that would search again in the
            // same content.
            WebFindOptions optionsForThisPass = onlyOneFrame ? options : (options & ~WebFindOptionsWrapAround);

            if (findString(searchView, string, optionsForThisPass)) {
                if (frame != startFrame)
                    [startFrame _clearSelection];
                [[self window] makeFirstResponder:searchView];
                return YES;
            }
            
            if (onlyOneFrame)
                return NO;
        }
        frame = nextFrame;
    } while (frame && frame != startFrame);
    
    // If there are multiple frames and WebFindOptionsWrapAround is set and we've visited each one without finding a result, we still need to search in the 
    // first-searched frame up to the selection. However, the API doesn't provide a way to search only up to a particular point. The only 
    // way to make sure the entire frame is searched is to pass WebFindOptionsWrapAround. When there are no matches, this will search
    // some content that we already searched on the first pass. In the worst case, we could search the entire contents of this frame twice.
    // To fix this, we'd need to add a mechanism to specify a range in which to search.
    if ((options & WebFindOptionsWrapAround) && startSearchView) {
        if (findString(startSearchView, string, options)) {
            [[self window] makeFirstResponder:startSearchView];
            return YES;
        }
    }
    return NO;
}

- (DOMRange *)DOMRangeOfString:(NSString *)string relativeTo:(DOMRange *)previousRange options:(WebFindOptions)options
{
    if (!_private->page)
        return nil;

    return kit(_private->page->rangeOfString(string, core(previousRange), coreOptions(options)).get());
}

- (void)setMainFrameDocumentReady:(BOOL)mainFrameDocumentReady
{
    // by setting this to NO, calls to mainFrameDocument are forced to return nil
    // setting this to YES lets it return the actual DOMDocument value
    // we use this to tell NSTreeController to reset its observers and clear its state
    if (_private->mainFrameDocumentReady == mainFrameDocumentReady)
        return;
#if !PLATFORM(IOS)
    [self _willChangeValueForKey:_WebMainFrameDocumentKey];
    _private->mainFrameDocumentReady = mainFrameDocumentReady;
    [self _didChangeValueForKey:_WebMainFrameDocumentKey];
    // this will cause observers to call mainFrameDocument where this flag will be checked
#endif
}

- (void)setTabKeyCyclesThroughElements:(BOOL)cyclesElements
{
    _private->tabKeyCyclesThroughElementsChanged = YES;
    if (_private->page)
        _private->page->setTabKeyCyclesThroughElements(cyclesElements);
}

- (BOOL)tabKeyCyclesThroughElements
{
    return _private->page && _private->page->tabKeyCyclesThroughElements();
}

- (void)setScriptDebugDelegate:(id)delegate
{
    _private->scriptDebugDelegate = delegate;
    [self _cacheScriptDebugDelegateImplementations];

    if (delegate)
        [self _attachScriptDebuggerToAllFrames];
    else
        [self _detachScriptDebuggerFromAllFrames];
}

- (id)scriptDebugDelegate
{
    return _private->scriptDebugDelegate;
}
  
- (void)setHistoryDelegate:(id)delegate
{
    _private->historyDelegate = delegate;
    [self _cacheHistoryDelegateImplementations];
}

- (id)historyDelegate
{
    return _private->historyDelegate;
}

- (BOOL)shouldClose
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return YES;
    return coreFrame->loader().shouldClose();
}

#if !PLATFORM(IOS)
static NSAppleEventDescriptor* aeDescFromJSValue(ExecState* exec, JSC::JSValue jsValue)
{
    NSAppleEventDescriptor* aeDesc = 0;
    if (jsValue.isBoolean())
        return [NSAppleEventDescriptor descriptorWithBoolean:jsValue.asBoolean()];
    if (jsValue.isString())
        return [NSAppleEventDescriptor descriptorWithString:jsValue.getString(exec)];
    if (jsValue.isNumber()) {
        double value = jsValue.asNumber();
        int intValue = value;
        if (value == intValue)
            return [NSAppleEventDescriptor descriptorWithDescriptorType:typeSInt32 bytes:&intValue length:sizeof(intValue)];
        return [NSAppleEventDescriptor descriptorWithDescriptorType:typeIEEE64BitFloatingPoint bytes:&value length:sizeof(value)];
    }
    if (jsValue.isObject()) {
        JSObject* object = jsValue.getObject();
        if (object->inherits(DateInstance::info())) {
            DateInstance* date = static_cast<DateInstance*>(object);
            double ms = date->internalNumber();
            if (!std::isnan(ms)) {
                CFAbsoluteTime utcSeconds = ms / 1000 - kCFAbsoluteTimeIntervalSince1970;
                LongDateTime ldt;
                if (noErr == UCConvertCFAbsoluteTimeToLongDateTime(utcSeconds, &ldt))
                    return [NSAppleEventDescriptor descriptorWithDescriptorType:typeLongDateTime bytes:&ldt length:sizeof(ldt)];
            }
        }
        else if (object->inherits(JSArray::info())) {
            DEPRECATED_DEFINE_STATIC_LOCAL(HashSet<JSObject*>, visitedElems, ());
            if (!visitedElems.contains(object)) {
                visitedElems.add(object);
                
                JSArray* array = static_cast<JSArray*>(object);
                aeDesc = [NSAppleEventDescriptor listDescriptor];
                unsigned numItems = array->length();
                for (unsigned i = 0; i < numItems; ++i)
                    [aeDesc insertDescriptor:aeDescFromJSValue(exec, array->get(exec, i)) atIndex:0];
                
                visitedElems.remove(object);
                return aeDesc;
            }
        }
        JSC::JSValue primitive = object->toPrimitive(exec);
        if (exec->hadException()) {
            exec->clearException();
            return [NSAppleEventDescriptor nullDescriptor];
        }
        return aeDescFromJSValue(exec, primitive);
    }
    if (jsValue.isUndefined())
        return [NSAppleEventDescriptor descriptorWithTypeCode:cMissingValue];
    ASSERT(jsValue.isNull());
    return [NSAppleEventDescriptor nullDescriptor];
}

- (NSAppleEventDescriptor *)aeDescByEvaluatingJavaScriptFromString:(NSString *)script
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    if (!coreFrame->document())
        return nil;
    JSC::JSValue result = coreFrame->script().executeScript(script, true).jsValue();
    if (!result) // FIXME: pass errors
        return 0;
    JSLockHolder lock(coreFrame->script().globalObject(mainThreadNormalWorld())->globalExec());
    return aeDescFromJSValue(coreFrame->script().globalObject(mainThreadNormalWorld())->globalExec(), result);
}
#endif

- (BOOL)canMarkAllTextMatches
{
    if (_private->closed)
        return NO;

    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if (view && ![view conformsToProtocol:@protocol(WebMultipleTextMatches)])
            return NO;
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return YES;
}

- (NSUInteger)countMatchesForText:(NSString *)string options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    return [self countMatchesForText:string inDOMRange:nil options:options highlight:highlight limit:limit markMatches:markMatches];
}

- (NSUInteger)countMatchesForText:(NSString *)string inDOMRange:(DOMRange *)range options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    if (_private->closed)
        return 0;

    WebFrame *frame = [self mainFrame];
    unsigned matchCount = 0;
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)]) {
            if (markMatches)
                [(NSView <WebMultipleTextMatches>*)view setMarkedTextMatchesAreHighlighted:highlight];
        
            ASSERT(limit == 0 || matchCount < limit);
            matchCount += [(NSView <WebMultipleTextMatches>*)view countMatchesForText:string inDOMRange:range options:options limit:(limit == 0 ? 0 : limit - matchCount) markMatches:markMatches];

            // Stop looking if we've reached the limit. A limit of 0 means no limit.
            if (limit > 0 && matchCount >= limit)
                break;
        }
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return matchCount;
}

- (void)unmarkAllTextMatches
{
    if (_private->closed)
        return;

    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)])
            [(NSView <WebMultipleTextMatches>*)view unmarkAllTextMatches];
        
        frame = incrementFrame(frame);
    } while (frame);
}

- (NSArray *)rectsForTextMatches
{
    if (_private->closed)
        return [NSArray array];

    NSMutableArray *result = [NSMutableArray array];
    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)]) {
            NSView <WebMultipleTextMatches> *documentView = (NSView <WebMultipleTextMatches> *)view;
            NSRect documentViewVisibleRect = [documentView visibleRect];
            NSArray *originalRects = [documentView rectsForTextMatches];
            unsigned rectCount = [originalRects count];
            unsigned rectIndex;
            NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
            for (rectIndex = 0; rectIndex < rectCount; ++rectIndex) {
                NSRect r = [[originalRects objectAtIndex:rectIndex] rectValue];
                // Clip rect to document view's visible rect so rect is confined to subframe
                r = NSIntersectionRect(r, documentViewVisibleRect);
                if (NSIsEmptyRect(r))
                    continue;
                
                // Convert rect to our coordinate system
                r = [documentView convertRect:r toView:self];
                [result addObject:[NSValue valueWithRect:r]];
                if (rectIndex % 10 == 0) {
                    [pool drain];
                    pool = [[NSAutoreleasePool alloc] init];
                }
            }
            [pool drain];
        }
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return result;
}

- (void)scrollDOMRangeToVisible:(DOMRange *)range
{
    [[[[range startContainer] ownerDocument] webFrame] _scrollDOMRangeToVisible:range];
}

#if PLATFORM(IOS)
- (void)scrollDOMRangeToVisible:(DOMRange *)range withInset:(CGFloat)inset
{
    [[[[range startContainer] ownerDocument] webFrame] _scrollDOMRangeToVisible:range withInset:inset];
}
#endif

- (BOOL)allowsUndo
{
    return _private->allowsUndo;
}

- (void)setAllowsUndo:(BOOL)flag
{
    _private->allowsUndo = flag;
}

- (void)setPageSizeMultiplier:(float)m
{
    [self _setZoomMultiplier:m isTextOnly:NO];
}

- (float)pageSizeMultiplier
{
    return ![self _realZoomMultiplierIsTextOnly] ? _private->zoomMultiplier : 1.0f;
}

- (BOOL)canZoomPageIn
{
    return [self _canZoomIn:NO];
}

- (IBAction)zoomPageIn:(id)sender
{
    return [self _zoomIn:sender isTextOnly:NO];
}

- (BOOL)canZoomPageOut
{
    return [self _canZoomOut:NO];
}

- (IBAction)zoomPageOut:(id)sender
{
    return [self _zoomOut:sender isTextOnly:NO];
}

- (BOOL)canResetPageZoom
{
    return [self _canResetZoom:NO];
}

- (IBAction)resetPageZoom:(id)sender
{
    return [self _resetZoom:sender isTextOnly:NO];
}

- (void)setMediaVolume:(float)volume
{
    if (_private->page)
        _private->page->setMediaVolume(volume);
}

- (float)mediaVolume
{
    if (!_private->page)
        return 0;

    return _private->page->mediaVolume();
}

- (void)addVisitedLinks:(NSArray *)visitedLinks
{
    WebVisitedLinkStore& visitedLinkStore = _private->group->visitedLinkStore();
    for (NSString *urlString in visitedLinks)
        visitedLinkStore.addVisitedLink(urlString);
}

#if PLATFORM(IOS)
- (void)removeVisitedLink:(NSURL *)url
{
    _private->group->visitedLinkStore().removeVisitedLink(URL(url).string());
}
#endif

@end

#if !PLATFORM(IOS)
@implementation WebView (WebViewPrintingPrivate)

- (float)_headerHeight
{
    return CallUIDelegateReturningFloat(self, @selector(webViewHeaderHeight:));
}

- (float)_footerHeight
{
    return CallUIDelegateReturningFloat(self, @selector(webViewFooterHeight:));
}

- (void)_drawHeaderInRect:(NSRect)rect
{
#ifdef DEBUG_HEADER_AND_FOOTER
    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];
    [[NSColor yellowColor] set];
    NSRectFill(rect);
    [currentContext restoreGraphicsState];
#endif

    SEL selector = @selector(webView:drawHeaderInRect:);
    if (![_private->UIDelegate respondsToSelector:selector])
        return;

    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];

    NSRectClip(rect);
    CallUIDelegate(self, selector, rect);

    [currentContext restoreGraphicsState];
}

- (void)_drawFooterInRect:(NSRect)rect
{
#ifdef DEBUG_HEADER_AND_FOOTER
    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];
    [[NSColor cyanColor] set];
    NSRectFill(rect);
    [currentContext restoreGraphicsState];
#endif
    
    SEL selector = @selector(webView:drawFooterInRect:);
    if (![_private->UIDelegate respondsToSelector:selector])
        return;

    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];

    NSRectClip(rect);
    CallUIDelegate(self, selector, rect);

    [currentContext restoreGraphicsState];
}

- (void)_adjustPrintingMarginsForHeaderAndFooter
{
    NSPrintOperation *op = [NSPrintOperation currentOperation];
    NSPrintInfo *info = [op printInfo];
    NSMutableDictionary *infoDictionary = [info dictionary];
    
    // We need to modify the top and bottom margins in the NSPrintInfo to account for the space needed by the
    // header and footer. Because this method can be called more than once on the same NSPrintInfo (see 5038087),
    // we stash away the unmodified top and bottom margins the first time this method is called, and we read from
    // those stashed-away values on subsequent calls.
    float originalTopMargin;
    float originalBottomMargin;
    NSNumber *originalTopMarginNumber = [infoDictionary objectForKey:WebKitOriginalTopPrintingMarginKey];
    if (!originalTopMarginNumber) {
        ASSERT(![infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey]);
        originalTopMargin = [info topMargin];
        originalBottomMargin = [info bottomMargin];
        [infoDictionary setObject:[NSNumber numberWithFloat:originalTopMargin] forKey:WebKitOriginalTopPrintingMarginKey];
        [infoDictionary setObject:[NSNumber numberWithFloat:originalBottomMargin] forKey:WebKitOriginalBottomPrintingMarginKey];
    } else {
        ASSERT([originalTopMarginNumber isKindOfClass:[NSNumber class]]);
        ASSERT([[infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey] isKindOfClass:[NSNumber class]]);
        originalTopMargin = [originalTopMarginNumber floatValue];
        originalBottomMargin = [[infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey] floatValue];
    }
    
    float scale = [op _web_pageSetupScaleFactor];
    [info setTopMargin:originalTopMargin + [self _headerHeight] * scale];
    [info setBottomMargin:originalBottomMargin + [self _footerHeight] * scale];
}

- (void)_drawHeaderAndFooter
{
    // The header and footer rect height scales with the page, but the width is always
    // all the way across the printed page (inset by printing margins).
    NSPrintOperation *op = [NSPrintOperation currentOperation];
    float scale = [op _web_pageSetupScaleFactor];
    NSPrintInfo *printInfo = [op printInfo];
    NSSize paperSize = [printInfo paperSize];
    float headerFooterLeft = [printInfo leftMargin]/scale;
    float headerFooterWidth = (paperSize.width - ([printInfo leftMargin] + [printInfo rightMargin]))/scale;
    NSRect footerRect = NSMakeRect(headerFooterLeft, [printInfo bottomMargin]/scale - [self _footerHeight] , 
                                   headerFooterWidth, [self _footerHeight]);
    NSRect headerRect = NSMakeRect(headerFooterLeft, (paperSize.height - [printInfo topMargin])/scale, 
                                   headerFooterWidth, [self _headerHeight]);
    
    [self _drawHeaderInRect:headerRect];
    [self _drawFooterInRect:footerRect];
}
@end

@implementation WebView (WebDebugBinding)

- (void)addObserver:(NSObject *)anObserver forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context
{
    LOG (Bindings, "addObserver:%p forKeyPath:%@ options:%x context:%p", anObserver, keyPath, options, context);
    [super addObserver:anObserver forKeyPath:keyPath options:options context:context];
}

- (void)removeObserver:(NSObject *)anObserver forKeyPath:(NSString *)keyPath
{
    LOG (Bindings, "removeObserver:%p forKeyPath:%@", anObserver, keyPath);
    [super removeObserver:anObserver forKeyPath:keyPath];
}

@end

#endif // !PLATFORM(IOS)

//==========================================================================================
// Editing

@implementation WebView (WebViewCSS)

- (DOMCSSStyleDeclaration *)computedStyleForElement:(DOMElement *)element pseudoElement:(NSString *)pseudoElement
{
    // FIXME: is this the best level for this conversion?
    if (pseudoElement == nil)
        pseudoElement = @"";

    return [[element ownerDocument] getComputedStyle:element pseudoElement:pseudoElement];
}

@end

@implementation WebView (WebViewEditing)

- (DOMRange *)editableDOMRangeForPoint:(NSPoint)point
{
    Page* page = core(self);
    if (!page)
        return nil;
    return kit(page->mainFrame().editor().rangeForPoint(IntPoint([self convertPoint:point toView:nil])).get());
}

- (BOOL)_shouldChangeSelectedDOMRange:(DOMRange *)currentRange toDOMRange:(DOMRange *)proposedRange affinity:(NSSelectionAffinity)selectionAffinity stillSelecting:(BOOL)flag
{
#if !PLATFORM(IOS)
    // FIXME: This quirk is needed due to <rdar://problem/4985321> - We can phase it out once Aperture can adopt the new behavior on their end
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_APERTURE_QUIRK) && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.Aperture"])
        return YES;
#endif
    return [[self _editingDelegateForwarder] webView:self shouldChangeSelectedDOMRange:currentRange toDOMRange:proposedRange affinity:selectionAffinity stillSelecting:flag];
}

- (void)_setMaintainsInactiveSelection:(BOOL)shouldMaintainInactiveSelection
{
    _private->shouldMaintainInactiveSelection = shouldMaintainInactiveSelection;
}

- (BOOL)maintainsInactiveSelection
{
    return _private->shouldMaintainInactiveSelection;
}

- (void)setSelectedDOMRange:(DOMRange *)range affinity:(NSSelectionAffinity)selectionAffinity
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return;

    if (range == nil)
        coreFrame->selection().clear();
    else {
        // Derive the frame to use from the range passed in.
        // Using _selectedOrMainFrame could give us a different document than
        // the one the range uses.
        coreFrame = core([range startContainer])->document().frame();
        if (!coreFrame)
            return;

        coreFrame->selection().setSelectedRange(core(range), core(selectionAffinity), true);
    }
}

- (DOMRange *)selectedDOMRange
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->selection().toNormalizedRange().get());
}

- (NSSelectionAffinity)selectionAffinity
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NSSelectionAffinityDownstream;
    return kit(coreFrame->selection().selection().affinity());
}

- (void)setEditable:(BOOL)flag
{
    if ([self isEditable] != flag && _private->page) {
        _private->page->setEditable(flag);
        if (!_private->tabKeyCyclesThroughElementsChanged)
            _private->page->setTabKeyCyclesThroughElements(!flag);
        Frame* mainFrame = [self _mainCoreFrame];
        if (mainFrame) {
            if (flag) {
                mainFrame->editor().applyEditingStyleToBodyElement();
                // If the WebView is made editable and the selection is empty, set it to something.
                if (![self selectedDOMRange])
                    mainFrame->selection().setSelectionFromNone();
            }
        }
    }
}

- (BOOL)isEditable
{
    return _private->page && _private->page->isEditable();
}

- (void)setTypingStyle:(DOMCSSStyleDeclaration *)style
{
    // We don't know enough at thls level to pass in a relevant WebUndoAction; we'd have to
    // change the API to allow this.
    [[self _selectedOrMainFrame] _setTypingStyle:style withUndoAction:EditActionUnspecified];
}

- (DOMCSSStyleDeclaration *)typingStyle
{
    return [[self _selectedOrMainFrame] _typingStyle];
}

- (void)setSmartInsertDeleteEnabled:(BOOL)flag
{
    if (_private->page->settings().smartInsertDeleteEnabled() != flag) {
        _private->page->settings().setSmartInsertDeleteEnabled(flag);
        [[NSUserDefaults standardUserDefaults] setBool:_private->page->settings().smartInsertDeleteEnabled() forKey:WebSmartInsertDeleteEnabled];
        [self setSelectTrailingWhitespaceEnabled:!flag];
    }
}

- (BOOL)smartInsertDeleteEnabled
{
    return _private->page->settings().smartInsertDeleteEnabled();
}

- (void)setContinuousSpellCheckingEnabled:(BOOL)flag
{
    if (continuousSpellCheckingEnabled == flag)
        return;

    continuousSpellCheckingEnabled = flag;
#if !PLATFORM(IOS)
    [[NSUserDefaults standardUserDefaults] setBool:continuousSpellCheckingEnabled forKey:WebContinuousSpellCheckingEnabled];
#endif
    if ([self isContinuousSpellCheckingEnabled])
        [[self class] _preflightSpellChecker];
    else
        [[self mainFrame] _unmarkAllMisspellings];
}

- (BOOL)isContinuousSpellCheckingEnabled
{
    return (continuousSpellCheckingEnabled && [self _continuousCheckingAllowed]);
}

#if !PLATFORM(IOS)
- (NSInteger)spellCheckerDocumentTag
{
    if (!_private->hasSpellCheckerDocumentTag) {
        _private->spellCheckerDocumentTag = [NSSpellChecker uniqueSpellDocumentTag];
        _private->hasSpellCheckerDocumentTag = YES;
    }
    return _private->spellCheckerDocumentTag;
}
#endif

- (NSUndoManager *)undoManager
{
    if (!_private->allowsUndo)
        return nil;

#if !PLATFORM(IOS)
    NSUndoManager *undoManager = [[self _editingDelegateForwarder] undoManagerForWebView:self];
    if (undoManager)
        return undoManager;

    return [super undoManager];
#else
    return [[self _editingDelegateForwarder] undoManagerForWebView:self];
#endif
}

- (void)registerForEditingDelegateNotification:(NSString *)name selector:(SEL)selector
{
    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    if ([_private->editingDelegate respondsToSelector:selector])
        [defaultCenter addObserver:_private->editingDelegate selector:selector name:name object:self];
}

- (void)setEditingDelegate:(id)delegate
{
    if (_private->editingDelegate == delegate)
        return;

    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];

    // remove notifications from current delegate
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidBeginEditingNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidEndEditingNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeTypingStyleNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeSelectionNotification object:self];
    
    _private->editingDelegate = delegate;
    [_private->editingDelegateForwarder release];
    _private->editingDelegateForwarder = nil;
    
    // add notifications for new delegate
    [self registerForEditingDelegateNotification:WebViewDidBeginEditingNotification selector:@selector(webViewDidBeginEditing:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeNotification selector:@selector(webViewDidChange:)];
    [self registerForEditingDelegateNotification:WebViewDidEndEditingNotification selector:@selector(webViewDidEndEditing:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeTypingStyleNotification selector:@selector(webViewDidChangeTypingStyle:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeSelectionNotification selector:@selector(webViewDidChangeSelection:)];
}

- (id)editingDelegate
{
    return _private->editingDelegate;
}

- (DOMCSSStyleDeclaration *)styleDeclarationWithText:(NSString *)text
{
    // FIXME: Should this really be attached to the document with the current selection?
    DOMCSSStyleDeclaration *decl = [[[self _selectedOrMainFrame] DOMDocument] createCSSStyleDeclaration];
    [decl setCssText:text];
    return decl;
}

@end

#if !PLATFORM(IOS)
@implementation WebView (WebViewGrammarChecking)

// FIXME: This method should be merged into WebViewEditing when we're not in API freeze
- (BOOL)isGrammarCheckingEnabled
{
    return grammarCheckingEnabled;
}

// FIXME: This method should be merged into WebViewEditing when we're not in API freeze
- (void)setGrammarCheckingEnabled:(BOOL)flag
{
    if (grammarCheckingEnabled == flag)
        return;
    
    grammarCheckingEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:grammarCheckingEnabled forKey:WebGrammarCheckingEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];

    // We call _preflightSpellChecker when turning continuous spell checking on, but we don't need to do that here
    // because grammar checking only occurs on code paths that already preflight spell checking appropriately.
    
    if (![self isGrammarCheckingEnabled])
        [[self mainFrame] _unmarkAllBadGrammar];
}

// FIXME: This method should be merged into WebIBActions when we're not in API freeze
- (void)toggleGrammarChecking:(id)sender
{
    [self setGrammarCheckingEnabled:![self isGrammarCheckingEnabled]];
}

@end
#endif

@implementation WebView (WebViewTextChecking)

- (BOOL)isAutomaticQuoteSubstitutionEnabled
{
#if PLATFORM(IOS)
    return NO;
#else
    return automaticQuoteSubstitutionEnabled;
#endif
}

- (BOOL)isAutomaticLinkDetectionEnabled
{
#if PLATFORM(IOS)
    return NO;
#else
    return automaticLinkDetectionEnabled;
#endif
}

- (BOOL)isAutomaticDashSubstitutionEnabled
{
#if PLATFORM(IOS)
    return NO;
#else
    return automaticDashSubstitutionEnabled;
#endif
}

- (BOOL)isAutomaticTextReplacementEnabled
{
#if PLATFORM(IOS)
    return NO;
#else
    return automaticTextReplacementEnabled;
#endif
}

- (BOOL)isAutomaticSpellingCorrectionEnabled
{
#if PLATFORM(IOS)
    return NO;
#else
    return automaticSpellingCorrectionEnabled;
#endif
}

#if !PLATFORM(IOS)

- (void)setAutomaticQuoteSubstitutionEnabled:(BOOL)flag
{
    if (automaticQuoteSubstitutionEnabled == flag)
        return;
    automaticQuoteSubstitutionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticQuoteSubstitutionEnabled forKey:WebAutomaticQuoteSubstitutionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticQuoteSubstitution:(id)sender
{
    [self setAutomaticQuoteSubstitutionEnabled:![self isAutomaticQuoteSubstitutionEnabled]];
}

- (void)setAutomaticLinkDetectionEnabled:(BOOL)flag
{
    if (automaticLinkDetectionEnabled == flag)
        return;
    automaticLinkDetectionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticLinkDetectionEnabled forKey:WebAutomaticLinkDetectionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticLinkDetection:(id)sender
{
    [self setAutomaticLinkDetectionEnabled:![self isAutomaticLinkDetectionEnabled]];
}

- (void)setAutomaticDashSubstitutionEnabled:(BOOL)flag
{
    if (automaticDashSubstitutionEnabled == flag)
        return;
    automaticDashSubstitutionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticDashSubstitutionEnabled forKey:WebAutomaticDashSubstitutionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticDashSubstitution:(id)sender
{
    [self setAutomaticDashSubstitutionEnabled:![self isAutomaticDashSubstitutionEnabled]];
}

- (void)setAutomaticTextReplacementEnabled:(BOOL)flag
{
    if (automaticTextReplacementEnabled == flag)
        return;
    automaticTextReplacementEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticTextReplacementEnabled forKey:WebAutomaticTextReplacementEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticTextReplacement:(id)sender
{
    [self setAutomaticTextReplacementEnabled:![self isAutomaticTextReplacementEnabled]];
}

- (void)setAutomaticSpellingCorrectionEnabled:(BOOL)flag
{
    if (automaticSpellingCorrectionEnabled == flag)
        return;
    automaticSpellingCorrectionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticSpellingCorrectionEnabled forKey:WebAutomaticSpellingCorrectionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticSpellingCorrection:(id)sender
{
    [self setAutomaticSpellingCorrectionEnabled:![self isAutomaticSpellingCorrectionEnabled]];
}

#endif // !PLATFORM(IOS)

@end

@implementation WebView (WebViewUndoableEditing)

- (void)replaceSelectionWithNode:(DOMNode *)node
{
    [[self _selectedOrMainFrame] _replaceSelectionWithNode:node selectReplacement:YES smartReplace:NO matchStyle:NO];
}    

- (void)replaceSelectionWithText:(NSString *)text
{
    [[self _selectedOrMainFrame] _replaceSelectionWithText:text selectReplacement:YES smartReplace:NO];
}

- (void)replaceSelectionWithMarkupString:(NSString *)markupString
{
    [[self _selectedOrMainFrame] _replaceSelectionWithMarkupString:markupString baseURLString:nil selectReplacement:YES smartReplace:NO];
}

- (void)replaceSelectionWithArchive:(WebArchive *)archive
{
    [[[self _selectedOrMainFrame] _dataSource] _replaceSelectionWithArchive:archive selectReplacement:YES];
}

- (void)deleteSelection
{
    WebFrame *webFrame = [self _selectedOrMainFrame];
    Frame* coreFrame = core(webFrame);
    if (coreFrame)
        coreFrame->editor().deleteSelectionWithSmartDelete([(WebHTMLView *)[[webFrame frameView] documentView] _canSmartCopyOrDelete]);
}
    
- (void)applyStyle:(DOMCSSStyleDeclaration *)style
{
    // We don't know enough at thls level to pass in a relevant WebUndoAction; we'd have to
    // change the API to allow this.
    WebFrame *webFrame = [self _selectedOrMainFrame];
    if (Frame* coreFrame = core(webFrame)) {
        // FIXME: We shouldn't have to make a copy here.
        Ref<MutableStyleProperties> properties(core(style)->copyProperties());
        coreFrame->editor().applyStyle(properties.ptr());
    }
}

@end

@implementation WebView (WebViewEditingActions)

- (void)_performResponderOperation:(SEL)selector with:(id)parameter
{
    static BOOL reentered = NO;
    if (reentered) {
        [[self nextResponder] tryToPerform:selector with:parameter];
        return;
    }

    // There are two possibilities here.
    //
    // One is that WebView has been called in its role as part of the responder chain.
    // In that case, it's fine to call the first responder and end up calling down the
    // responder chain again. Later we will return here with reentered = YES and continue
    // past the WebView.
    //
    // The other is that we are being called directly, in which case we want to pass the
    // selector down to the view inside us that can handle it, and continue down the
    // responder chain as usual.

    // Pass this selector down to the first responder.
    NSResponder *responder = [self _responderForResponderOperations];
    reentered = YES;
    [responder tryToPerform:selector with:parameter];
    reentered = NO;
}

#define FORWARD(name) \
    - (void)name:(id)sender { [self _performResponderOperation:_cmd with:sender]; }

FOR_EACH_RESPONDER_SELECTOR(FORWARD)

#if PLATFORM(IOS)
FORWARD(clearText)
FORWARD(toggleBold)
FORWARD(toggleItalic)
FORWARD(toggleUnderline)

- (void)insertDictationPhrases:(NSArray *)dictationPhrases metadata:(id)metadata
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return;
    
    coreFrame->editor().insertDictationPhrases(vectorForDictationPhrasesArray(dictationPhrases), metadata);
}
#endif

- (void)insertText:(NSString *)text
{
    [self _performResponderOperation:_cmd with:text];
}

- (NSDictionary *)typingAttributes
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (coreFrame)
        return coreFrame->editor().fontAttributesForSelectionStart();
    
    return nil;
}


@end

@implementation WebView (WebViewEditingInMail)

- (void)_insertNewlineInQuotedContent
{
    [[self _selectedOrMainFrame] _insertParagraphSeparatorInQuotedContent];
}

- (void)_replaceSelectionWithNode:(DOMNode *)node matchStyle:(BOOL)matchStyle
{
    [[self _selectedOrMainFrame] _replaceSelectionWithNode:node selectReplacement:YES smartReplace:NO matchStyle:matchStyle];
}

- (BOOL)_selectionIsCaret
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NO;
    return coreFrame->selection().isCaret();
}

- (BOOL)_selectionIsAll
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NO;
    return coreFrame->selection().isAll(CanCrossEditingBoundary);
}

- (void)_simplifyMarkup:(DOMNode *)startNode endNode:(DOMNode *)endNode
{
    Frame* coreFrame = core([self mainFrame]);
    if (!coreFrame || !startNode)
        return;
    Node* coreStartNode= core(startNode);
    if (&coreStartNode->document() != coreFrame->document())
        return;
    return coreFrame->editor().simplifyMarkup(coreStartNode, core(endNode));    
}

@end

static WebFrameView *containingFrameView(NSView *view)
{
    while (view && ![view isKindOfClass:[WebFrameView class]])
        view = [view superview];
    return (WebFrameView *)view;    
}

@implementation WebView (WebFileInternal)

#if !PLATFORM(IOS)
- (float)_deviceScaleFactor
{
    if (_private->customDeviceScaleFactor != 0)
        return _private->customDeviceScaleFactor;

    NSWindow *window = [self window];
    NSWindow *hostWindow = [self hostWindow];
    if (window)
        return [window backingScaleFactor];
    if (hostWindow)
        return [hostWindow backingScaleFactor];
    return [[NSScreen mainScreen] backingScaleFactor];
}
#endif

static inline uint64_t roundUpToPowerOf2(uint64_t num)
{
    return powf(2.0, ceilf(log2f(num)));
}

+ (void)_setCacheModel:(WebCacheModel)cacheModel
{
    if (s_didSetCacheModel && cacheModel == s_cacheModel)
        return;

    NSString *nsurlCacheDirectory = CFBridgingRelease(WKCopyFoundationCacheDirectory());
    if (!nsurlCacheDirectory)
        nsurlCacheDirectory = NSHomeDirectory();

    static uint64_t memSize = roundUpToPowerOf2(WebMemorySize() / 1024 / 1024);
    unsigned long long diskFreeSize = WebVolumeFreeSize(nsurlCacheDirectory) / 1024 / 1000;
    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    auto deadDecodedDataDeletionInterval = std::chrono::seconds { 0 };

    unsigned pageCacheSize = 0;

    NSUInteger nsurlCacheMemoryCapacity = 0;
    NSUInteger nsurlCacheDiskCapacity = 0;
#if PLATFORM(IOS)
    unsigned tileLayerPoolCapacity = 0;
#endif

    switch (cacheModel) {
    case WebCacheModelDocumentViewer: {
        // Page cache capacity (in pages)
        pageCacheSize = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 4096)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;
#if PLATFORM(IOS)
        else
            cacheTotalCapacity = 4 * 1024 * 1024; 
#endif

        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;

        // Foundation memory cache capacity (in bytes)
        nsurlCacheMemoryCapacity = 0;

        // Foundation disk cache capacity (in bytes)
        nsurlCacheDiskCapacity = [nsurlCache diskCapacity];

#if PLATFORM(IOS)
        // TileCache layer pool capacity, in bytes.
        if (memSize >= 1024)
            tileLayerPoolCapacity = 24 * 1024 * 1024;
        else
            tileLayerPoolCapacity = 12 * 1024 * 1024;
#endif
        break;
    }
    case WebCacheModelDocumentBrowser: {
        // Page cache capacity (in pages)
        if (memSize >= 1024)
            pageCacheSize = 3;
        else if (memSize >= 512)
            pageCacheSize = 2;
        else if (memSize >= 256)
            pageCacheSize = 1;
        else
            pageCacheSize = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 4096)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;

        // Foundation memory cache capacity (in bytes)
        if (memSize >= 2048)
            nsurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 1024)
            nsurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 512)
            nsurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            nsurlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            nsurlCacheDiskCapacity = 50 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            nsurlCacheDiskCapacity = 40 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            nsurlCacheDiskCapacity = 30 * 1024 * 1024;
        else
            nsurlCacheDiskCapacity = 20 * 1024 * 1024;

#if PLATFORM(IOS)
        // TileCache layer pool capacity, in bytes.
        if (memSize >= 1024)
            tileLayerPoolCapacity = 24 * 1024 * 1024;
        else
            tileLayerPoolCapacity = 12 * 1024 * 1024;
#endif
        break;
    }
    case WebCacheModelPrimaryWebBrowser: {
        // Page cache capacity (in pages)
        // (Research indicates that value / page drops substantially after 3 pages.)
        if (memSize >= 2048)
            pageCacheSize = 5;
        else if (memSize >= 1024)
            pageCacheSize = 4;
        else if (memSize >= 512)
            pageCacheSize = 3;
        else if (memSize >= 256)
            pageCacheSize = 2;
        else
            pageCacheSize = 1;

#if PLATFORM(IOS)
        // Cache page less aggressively in iOS to reduce the chance of being jettisoned.
        // FIXME (<rdar://problem/11779846>): Avoiding jettisoning should not have to require reducing the page cache capacity.
        // Reducing the capacity by 1 reduces overall back-forward performance.
        if (pageCacheSize > 0)
            pageCacheSize -= 1;
#endif

        // Object cache capacities (in bytes)
        // (Testing indicates that value / MB depends heavily on content and
        // browsing pattern. Even growth above 128MB can have substantial 
        // value / MB for some content / browsing patterns.)
        if (memSize >= 4096)
            cacheTotalCapacity = 192 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 32 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;

        // This code is here to avoid a PLT regression. We can remove it if we
        // can prove that the overall system gain would justify the regression.
        cacheMaxDeadCapacity = std::max<unsigned>(24, cacheMaxDeadCapacity);

        deadDecodedDataDeletionInterval = std::chrono::seconds { 60 };

#if PLATFORM(IOS)
        if (memSize >= 1024)
            nsurlCacheMemoryCapacity = 16 * 1024 * 1024;
        else
            nsurlCacheMemoryCapacity = 8 * 1024 * 1024;
#else
        // Foundation memory cache capacity (in bytes)
        // (These values are small because WebCore does most caching itself.)
        if (memSize >= 1024)
            nsurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 512)
            nsurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 256)
            nsurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            nsurlCacheMemoryCapacity =      512 * 1024; 
#endif

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            nsurlCacheDiskCapacity = 175 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            nsurlCacheDiskCapacity = 150 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            nsurlCacheDiskCapacity = 125 * 1024 * 1024;
        else if (diskFreeSize >= 2048)
            nsurlCacheDiskCapacity = 100 * 1024 * 1024;
        else if (diskFreeSize >= 1024)
            nsurlCacheDiskCapacity = 75 * 1024 * 1024;
        else
            nsurlCacheDiskCapacity = 50 * 1024 * 1024;

#if PLATFORM(IOS)
        // TileCache layer pool capacity, in bytes.
        if (memSize >= 1024)
            tileLayerPoolCapacity = 48 * 1024 * 1024;
        else
            tileLayerPoolCapacity = 24 * 1024 * 1024;
#endif
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    };


    // Don't shrink a big disk cache, since that would cause churn.
    nsurlCacheDiskCapacity = std::max(nsurlCacheDiskCapacity, [nsurlCache diskCapacity]);

    auto& memoryCache = MemoryCache::singleton();
    memoryCache.setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache.setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);

    auto& pageCache = PageCache::singleton();
    pageCache.setMaxSize(pageCacheSize);
#if PLATFORM(IOS)
    pageCache.setShouldClearBackingStores(true);
    nsurlCacheMemoryCapacity = std::max(nsurlCacheMemoryCapacity, [nsurlCache memoryCapacity]);
    CFURLCacheRef cfCache;
    if ([nsurlCache respondsToSelector:@selector(_CFURLCache)] && (cfCache = [nsurlCache _CFURLCache]))
        CFURLCacheSetMemoryCapacity(cfCache, nsurlCacheMemoryCapacity);
    else
        [nsurlCache setMemoryCapacity:nsurlCacheMemoryCapacity];
#else
    [nsurlCache setMemoryCapacity:nsurlCacheMemoryCapacity];
#endif
    [nsurlCache setDiskCapacity:nsurlCacheDiskCapacity];

#if PLATFORM(IOS)
    [WebView _setTileCacheLayerPoolCapacity:tileLayerPoolCapacity];
#endif

    s_cacheModel = cacheModel;
    s_didSetCacheModel = YES;
}

+ (WebCacheModel)_cacheModel
{
    return s_cacheModel;
}

+ (BOOL)_didSetCacheModel
{
    return s_didSetCacheModel;
}

+ (WebCacheModel)_maxCacheModelInAnyInstance
{
    WebCacheModel cacheModel = WebCacheModelDocumentViewer;
    NSEnumerator *enumerator = [(NSMutableSet *)allWebViewsSet objectEnumerator];
    while (WebPreferences *preferences = [[enumerator nextObject] preferences])
        cacheModel = std::max(cacheModel, [preferences cacheModel]);
    return cacheModel;
}

+ (void)_cacheModelChangedNotification:(NSNotification *)notification
{
#if PLATFORM(IOS)
    // This needs to happen on the Web Thread
    WebThreadRun(^{
#endif    
    WebPreferences *preferences = (WebPreferences *)[notification object];
    ASSERT([preferences isKindOfClass:[WebPreferences class]]);

    WebCacheModel cacheModel = [preferences cacheModel];
    if (![self _didSetCacheModel] || cacheModel > [self _cacheModel])
        [self _setCacheModel:cacheModel];
    else if (cacheModel < [self _cacheModel])
        [self _setCacheModel:std::max([[WebPreferences standardPreferences] cacheModel], [self _maxCacheModelInAnyInstance])];
#if PLATFORM(IOS)
    });
#endif
}

+ (void)_preferencesRemovedNotification:(NSNotification *)notification
{
    WebPreferences *preferences = (WebPreferences *)[notification object];
    ASSERT([preferences isKindOfClass:[WebPreferences class]]);

    if ([preferences cacheModel] == [self _cacheModel])
        [self _setCacheModel:std::max([[WebPreferences standardPreferences] cacheModel], [self _maxCacheModelInAnyInstance])];
}

- (WebFrame *)_focusedFrame
{
    NSResponder *resp = [[self window] firstResponder];
    if (resp && [resp isKindOfClass:[NSView class]] && [(NSView *)resp isDescendantOf:[[self mainFrame] frameView]]) {
        WebFrameView *frameView = containingFrameView((NSView *)resp);
        ASSERT(frameView != nil);
        return [frameView webFrame];
    }
    
    return nil;
}

- (BOOL)_isLoading
{
    WebFrame *mainFrame = [self mainFrame];
    return [[mainFrame _dataSource] isLoading]
        || [[mainFrame provisionalDataSource] isLoading];
}

- (WebFrameView *)_frameViewAtWindowPoint:(NSPoint)point
{
    if (_private->closed)
        return nil;
#if !PLATFORM(IOS)
    NSView *view = [self hitTest:[[self superview] convertPoint:point fromView:nil]];
#else
    //[WebView superview] on iOS is nil, don't do a convertPoint
    NSView *view = [self hitTest:point];    
#endif
    if (![view isDescendantOf:[[self mainFrame] frameView]])
        return nil;
    WebFrameView *frameView = containingFrameView(view);
    ASSERT(frameView);
    return frameView;
}

+ (void)_preflightSpellCheckerNow:(id)sender
{
#if !PLATFORM(IOS)
    [[NSSpellChecker sharedSpellChecker] _preflightChosenSpellServer];
#endif
}

+ (void)_preflightSpellChecker
{
#if !PLATFORM(IOS)
    // As AppKit does, we wish to delay tickling the shared spellchecker into existence on application launch.
    if ([NSSpellChecker sharedSpellCheckerExists]) {
        [self _preflightSpellCheckerNow:self];
    } else {
        [self performSelector:@selector(_preflightSpellCheckerNow:) withObject:self afterDelay:2.0];
    }
#endif
}

- (BOOL)_continuousCheckingAllowed
{
    static BOOL allowContinuousSpellChecking = YES;
    static BOOL readAllowContinuousSpellCheckingDefault = NO;
    if (!readAllowContinuousSpellCheckingDefault) {
        if ([[NSUserDefaults standardUserDefaults] objectForKey:@"NSAllowContinuousSpellChecking"]) {
            allowContinuousSpellChecking = [[NSUserDefaults standardUserDefaults] boolForKey:@"NSAllowContinuousSpellChecking"];
        }
        readAllowContinuousSpellCheckingDefault = YES;
    }
    return allowContinuousSpellChecking;
}

- (NSResponder *)_responderForResponderOperations
{
    NSResponder *responder = [[self window] firstResponder];
    WebFrameView *mainFrameView = [[self mainFrame] frameView];
    
    // If the current responder is outside of the webview, use our main frameView or its
    // document view. We also do this for subviews of self that are siblings of the main
    // frameView since clients might insert non-webview-related views there (see 4552713).
    if (responder != self && ![mainFrameView _web_firstResponderIsSelfOrDescendantView]) {
        responder = [mainFrameView documentView];
        if (!responder)
            responder = mainFrameView;
    }
    return responder;
}

#if !PLATFORM(IOS)
- (void)_openFrameInNewWindowFromMenu:(NSMenuItem *)sender
{
    ASSERT_ARG(sender, [sender isKindOfClass:[NSMenuItem class]]);

    NSDictionary *element = [sender representedObject];
    ASSERT([element isKindOfClass:[NSDictionary class]]);

    WebDataSource *dataSource = [(WebFrame *)[element objectForKey:WebElementFrameKey] dataSource];
    NSURLRequest *request = [[dataSource request] copy];
    ASSERT(request);
    
    [self _openNewWindowWithRequest:request];
    [request release];
}

- (void)_searchWithGoogleFromMenu:(id)sender
{
    id documentView = [[[self selectedFrame] frameView] documentView];
    if (![documentView conformsToProtocol:@protocol(WebDocumentText)]) {
        return;
    }
    
    NSString *selectedString = [(id <WebDocumentText>)documentView selectedString];
    if ([selectedString length] == 0) {
        return;
    }
    
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithUniqueName];
    [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    NSMutableString *s = [selectedString mutableCopy];
    const unichar nonBreakingSpaceCharacter = 0xA0;
    NSString *nonBreakingSpaceString = [NSString stringWithCharacters:&nonBreakingSpaceCharacter length:1];
    [s replaceOccurrencesOfString:nonBreakingSpaceString withString:@" " options:0 range:NSMakeRange(0, [s length])];
    [pasteboard setString:s forType:NSStringPboardType];
    [s release];
    
    // FIXME: seems fragile to use the service by name, but this is what AppKit does
    NSPerformService(@"Search With Google", pasteboard);
}

- (void)_searchWithSpotlightFromMenu:(id)sender
{
    id documentView = [[[self selectedFrame] frameView] documentView];
    if (![documentView conformsToProtocol:@protocol(WebDocumentText)])
        return;

    NSString *selectedString = [(id <WebDocumentText>)documentView selectedString];
    if (![selectedString length])
        return;

    [[NSWorkspace sharedWorkspace] showSearchResultsForQueryString:selectedString];
}
#endif // !PLATFORM(IOS)

@end

@implementation WebView (WebViewInternal)

+ (BOOL)shouldIncludeInWebKitStatistics
{
    return NO;
}

- (BOOL)_becomingFirstResponderFromOutside
{
    return _private->becomingFirstResponderFromOutside;
}

#if ENABLE(ICONDATABASE)
- (void)_receivedIconChangedNotification:(NSNotification *)notification
{
    // Get the URL for this notification
    NSDictionary *userInfo = [notification userInfo];
    ASSERT([userInfo isKindOfClass:[NSDictionary class]]);
    NSString *urlString = [userInfo objectForKey:WebIconNotificationUserInfoURLKey];
    ASSERT([urlString isKindOfClass:[NSString class]]);
    
    // If that URL matches the current main frame, dispatch the delegate call, which will also unregister
    // us for this notification
    if ([[self mainFrameURL] isEqualTo:urlString])
        [self _dispatchDidReceiveIconFromWebFrame:[self mainFrame]];
}

- (void)_registerForIconNotification:(BOOL)listen
{
    if (listen)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_receivedIconChangedNotification:) name:WebIconDatabaseDidAddIconNotification object:nil];        
    else
        [[NSNotificationCenter defaultCenter] removeObserver:self name:WebIconDatabaseDidAddIconNotification object:nil];
}

- (void)_dispatchDidReceiveIconFromWebFrame:(WebFrame *)webFrame
{
    // FIXME: This willChangeValueForKey call is too late, because the icon has already changed by now.
    [self _willChangeValueForKey:_WebMainFrameIconKey];
    
    // Since we definitely have an icon and are about to send out the delegate call for that, this WebView doesn't need to listen for the general
    // notification any longer
    [self _registerForIconNotification:NO];

    WebFrameLoadDelegateImplementationCache* cache = &_private->frameLoadDelegateImplementations;
    if (cache->didReceiveIconForFrameFunc) {
        Image* image = iconDatabase().synchronousIconForPageURL(core(webFrame)->document()->url().string(), IntSize(16, 16));
        if (NSImage *icon = webGetNSImage(image, NSMakeSize(16, 16)))
            CallFrameLoadDelegate(cache->didReceiveIconForFrameFunc, self, @selector(webView:didReceiveIcon:forFrame:), icon, webFrame);
    }

    [self _didChangeValueForKey:_WebMainFrameIconKey];
}
#endif // ENABLE(ICONDATABASE)

- (void)_addObject:(id)object forIdentifier:(unsigned long)identifier
{
    ASSERT(!_private->identifierMap.contains(identifier));

    // If the identifier map is initially empty it means we're starting a load
    // of something. The semantic is that the web view should be around as long 
    // as something is loading. Because of that we retain the web view.
    if (_private->identifierMap.isEmpty())
        CFRetain(self);
    
    _private->identifierMap.set(identifier, object);
}

- (id)_objectForIdentifier:(unsigned long)identifier
{
    return _private->identifierMap.get(identifier).get();
}

- (void)_removeObjectForIdentifier:(unsigned long)identifier
{
    ASSERT(_private->identifierMap.contains(identifier));
    _private->identifierMap.remove(identifier);
    
    // If the identifier map is now empty it means we're no longer loading anything
    // and we should release the web view. Autorelease rather than release in order to
    // avoid re-entering this method beneath -dealloc with the same identifier. <rdar://problem/10523721>
    if (_private->identifierMap.isEmpty())
        [self autorelease];
}

- (void)_retrieveKeyboardUIModeFromPreferences:(NSNotification *)notification
{
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

    Boolean keyExistsAndHasValidFormat;
    int mode = CFPreferencesGetAppIntegerValue(AppleKeyboardUIMode, kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);

    // The keyboard access mode has two bits:
    // Bit 0 is set if user can set the focus to menus, the dock, and various windows using the keyboard.
    // Bit 1 is set if controls other than text fields are included in the tab order (WebKit also always includes lists).
    _private->_keyboardUIMode = (mode & 0x2) ? KeyboardAccessFull : KeyboardAccessDefault;
#if !PLATFORM(IOS)
    // check for tabbing to links
    if ([_private->preferences tabsToLinks])
        _private->_keyboardUIMode = (KeyboardUIMode)(_private->_keyboardUIMode | KeyboardAccessTabsToLinks);
#endif
}

- (KeyboardUIMode)_keyboardUIMode
{
    if (!_private->_keyboardUIModeAccessed) {
        _private->_keyboardUIModeAccessed = YES;

        [self _retrieveKeyboardUIModeFromPreferences:nil];
        
#if !PLATFORM(IOS)
        [[NSDistributedNotificationCenter defaultCenter] 
            addObserver:self selector:@selector(_retrieveKeyboardUIModeFromPreferences:) 
            name:KeyboardUIModeDidChangeNotification object:nil];
#endif

        [[NSNotificationCenter defaultCenter] 
            addObserver:self selector:@selector(_retrieveKeyboardUIModeFromPreferences:) 
            name:WebPreferencesChangedInternalNotification object:nil];
    }
    return _private->_keyboardUIMode;
}

#if !PLATFORM(IOS)
- (void)_setInsertionPasteboard:(NSPasteboard *)pasteboard
{
    _private->insertionPasteboard = pasteboard;
}
#endif

- (Frame*)_mainCoreFrame
{
    return (_private && _private->page) ? &_private->page->mainFrame() : 0;
}

- (WebFrame *)_selectedOrMainFrame
{
    WebFrame *result = [self selectedFrame];
    if (result == nil)
        result = [self mainFrame];
    return result;
}

- (BOOL)_needsOneShotDrawingSynchronization
{
    return _private->needsOneShotDrawingSynchronization;
}

- (void)_setNeedsOneShotDrawingSynchronization:(BOOL)needsSynchronization
{
    _private->needsOneShotDrawingSynchronization = needsSynchronization;
}

/*
    The order of events with compositing updates is this:
    
   Start of runloop                                        End of runloop
        |                                                       |
      --|-------------------------------------------------------|--
           ^         ^                                        ^
           |         |                                        |
    NSWindow update, |                                     CA commit
     NSView drawing  |                                  
        flush        |                                  
                layerSyncRunLoopObserverCallBack

    To avoid flashing, we have to ensure that compositing changes (rendered via
    the CoreAnimation rendering display link) appear on screen at the same time
    as content painted into the window via the normal WebCore rendering path.

    CoreAnimation will commit any layer changes at the end of the runloop via
    its "CA commit" observer. Those changes can then appear onscreen at any time
    when the display link fires, which can result in unsynchronized rendering.
    
    To fix this, the GraphicsLayerCA code in WebCore does not change the CA
    layer tree during style changes and layout; it stores up all changes and
    commits them via flushCompositingState(). There are then two situations in
    which we can call flushCompositingState():
    
    1. When painting. FrameView::paintContents() makes a call to flushCompositingState().
    
    2. When style changes/layout have made changes to the layer tree which do not
       result in painting. In this case we need a run loop observer to do a
       flushCompositingState() at an appropriate time. The observer will keep firing
       until the time is right (essentially when there are no more pending layouts).
    
*/
bool LayerFlushController::flushLayers()
{
#if PLATFORM(IOS)
    WebThreadLock();
#endif
#if !PLATFORM(IOS)
    NSWindow *window = [m_webView window];
    
    // An NSWindow may not display in the next runloop cycle after dirtying due to delayed window display logic,
    // in which case this observer can fire first. So if the window is due for a display, don't commit
    // layer changes, otherwise they'll show on screen before the view drawing.
    bool viewsNeedDisplay;
#ifndef __LP64__
    if (window && [window _wrapsCarbonWindow])
        viewsNeedDisplay = HIViewGetNeedsDisplay(HIViewGetRoot(static_cast<WindowRef>([window windowRef])));
    else
#endif
        viewsNeedDisplay = [window viewsNeedDisplay];

    if (viewsNeedDisplay)
        return false;
#endif

#if PLATFORM(IOS)
    // Ensure fixed positions layers are where they should be.
    [m_webView _synchronizeCustomFixedPositionLayoutRect];
#endif

    [m_webView _viewWillDrawInternal];

    if ([m_webView _flushCompositingChanges]) {
#if !PLATFORM(IOS)
        // AppKit may have disabled screen updates, thinking an upcoming window flush will re-enable them.
        // In case setNeedsDisplayInRect() has prevented the window from needing to be flushed, re-enable screen
        // updates here.
        if (![window isFlushWindowDisabled])
            [window _enableScreenUpdatesIfNeeded];
#endif

        return true;
    }

    return false;
}

- (void)_scheduleCompositingLayerFlush
{
#if PLATFORM(IOS)
    if (_private->closing)
        return;
#endif

    if (!_private->layerFlushController)
        _private->layerFlushController = LayerFlushController::create(self);
    _private->layerFlushController->scheduleLayerFlush();
}

- (BOOL)_flushCompositingChanges
{
    Frame* frame = [self _mainCoreFrame];
    if (frame && frame->view())
        return frame->view()->flushCompositingStateIncludingSubframes();

    return YES;
}

#if PLATFORM(IOS)
- (void)_scheduleLayerFlushForPendingTileCacheRepaint
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        view->scheduleLayerFlushAllowingThrottling();
}
#endif

#if ENABLE(VIDEO)
- (void)_enterVideoFullscreenForVideoElement:(WebCore::HTMLVideoElement*)videoElement mode:(WebCore::HTMLMediaElement::VideoFullscreenMode)mode
{
    if (_private->fullscreenController) {
        if ([_private->fullscreenController videoElement] == videoElement) {
            // The backend may just warn us that the underlaying plaftormMovie()
            // has changed. Just force an update.
            [_private->fullscreenController setVideoElement:videoElement];
            return; // No more to do.
        }

        // First exit Fullscreen for the old videoElement.
        [_private->fullscreenController videoElement]->exitFullscreen();
        // This previous call has to trigger _exitFullscreen,
        // which has to clear _private->fullscreenController.
        ASSERT(!_private->fullscreenController);
    }
    if (!_private->fullscreenController) {
        _private->fullscreenController = [[WebVideoFullscreenController alloc] init];
        [_private->fullscreenController setVideoElement:videoElement];
#if PLATFORM(IOS)
        [_private->fullscreenController enterFullscreen:(UIView *)[[[self window] hostLayer] delegate] mode:mode];
#else
        [_private->fullscreenController enterFullscreen:[[self window] screen]];
#endif
    }
    else
        [_private->fullscreenController setVideoElement:videoElement];
}

- (void)_exitVideoFullscreen
{
    if (!_private->fullscreenController)
        return;
    [_private->fullscreenController exitFullscreen];
    [_private->fullscreenController release];
    _private->fullscreenController = nil;
}

#endif // ENABLE(VIDEO)

#if ENABLE(FULLSCREEN_API) && !PLATFORM(IOS)
- (BOOL)_supportsFullScreenForElement:(const WebCore::Element*)element withKeyboard:(BOOL)withKeyboard
{
    if (![[self preferences] fullScreenEnabled])
        return NO;

    return !withKeyboard;
}

- (void)_enterFullScreenForElement:(WebCore::Element*)element
{
    if (!_private->newFullscreenController)
        _private->newFullscreenController = [[WebFullScreenController alloc] init];

    [_private->newFullscreenController setElement:element];
    [_private->newFullscreenController setWebView:self];
    [_private->newFullscreenController enterFullScreen:[[self window] screen]];        
}

- (void)_exitFullScreenForElement:(WebCore::Element*)element
{
    if (!_private->newFullscreenController)
        return;
    [_private->newFullscreenController exitFullScreen];
}
#endif

#if USE(AUTOCORRECTION_PANEL)
- (void)handleAcceptedAlternativeText:(NSString*)text
{
    WebFrame *webFrame = [self _selectedOrMainFrame];
    Frame* coreFrame = core(webFrame);
    if (coreFrame)
        coreFrame->editor().handleAlternativeTextUIResult(text);
}
#endif

#if USE(DICTATION_ALTERNATIVES)
- (void)_getWebCoreDictationAlternatives:(Vector<DictationAlternative>&)alternatives fromTextAlternatives:(const Vector<TextAlternativeWithRange>&)alternativesWithRange
{
    for (size_t i = 0; i < alternativesWithRange.size(); ++i) {
        const TextAlternativeWithRange& alternativeWithRange = alternativesWithRange[i];
        uint64_t dictationContext = _private->m_alternativeTextUIController->addAlternatives(alternativeWithRange.alternatives);
        if (dictationContext)
            alternatives.append(DictationAlternative(alternativeWithRange.range.location, alternativeWithRange.range.length, dictationContext));
    }
}

- (void)_showDictationAlternativeUI:(const WebCore::FloatRect&)boundingBoxOfDictatedText forDictationContext:(uint64_t)dictationContext
{
    _private->m_alternativeTextUIController->showAlternatives(self, [self _convertRectFromRootView:boundingBoxOfDictatedText], dictationContext, ^(NSString* acceptedAlternative) {
        [self handleAcceptedAlternativeText:acceptedAlternative];
    });
}

- (void)_removeDictationAlternatives:(uint64_t)dictationContext
{
    _private->m_alternativeTextUIController->removeAlternatives(dictationContext);
}

- (Vector<String>)_dictationAlternatives:(uint64_t)dictationContext
{
    return _private->m_alternativeTextUIController->alternativesForContext(dictationContext);
}
#endif

#if ENABLE(SERVICE_CONTROLS)
- (WebSelectionServiceController&)_selectionServiceController
{
    if (!_private->_selectionServiceController)
        _private->_selectionServiceController = std::make_unique<WebSelectionServiceController>(self);
    return *_private->_selectionServiceController;
}
#endif

- (NSPoint)_convertPointFromRootView:(NSPoint)point
{
    return NSMakePoint(point.x, [self bounds].size.height - point.y);
}

- (NSRect)_convertRectFromRootView:(NSRect)rect
{
#if PLATFORM(MAC)
    if (self.isFlipped)
        return rect;
#endif
    return NSMakeRect(rect.origin.x, [self bounds].size.height - rect.origin.y - rect.size.height, rect.size.width, rect.size.height);
}

#if PLATFORM(MAC)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
- (void)prepareForMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    if (menu != self._actionMenu)
        return;

    [_private->actionMenuController prepareForMenu:menu withEvent:event];
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    if (menu != self._actionMenu)
        return;

    [_private->actionMenuController willOpenMenu:menu withEvent:event];
}

- (void)didCloseMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    if (menu != self._actionMenu)
        return;

    [_private->actionMenuController didCloseMenu:menu withEvent:event];
}

- (WebActionMenuController *)_actionMenuController
{
    return _private->actionMenuController;
}

- (WebImmediateActionController *)_immediateActionController
{
    return _private->immediateActionController;
}

- (id)_animationControllerForDictionaryLookupPopupInfo:(const DictionaryPopupInfo&)dictionaryPopupInfo
{
    if (!dictionaryPopupInfo.attributedString)
        return nil;

    NSPoint textBaselineOrigin = dictionaryPopupInfo.origin;

    // Convert to screen coordinates.
    textBaselineOrigin = [self.window convertRectToScreen:NSMakeRect(textBaselineOrigin.x, textBaselineOrigin.y, 0, 0)].origin;

    if (canLoadLUTermOptionDisableSearchTermIndicator() && canLoadLUNotificationPopoverWillClose()) {
        if (!_private->hasInitializedLookupObserver) {
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_dictionaryLookupPopoverWillClose:) name:getLUNotificationPopoverWillClose() object:nil];
            _private->hasInitializedLookupObserver = YES;
        }

        RetainPtr<NSMutableDictionary> mutableOptions = adoptNS([dictionaryPopupInfo.options mutableCopy]);
        if (!mutableOptions)
            mutableOptions = adoptNS([[NSMutableDictionary alloc] init]);
        [mutableOptions setObject:@YES forKey:getLUTermOptionDisableSearchTermIndicator()];
        [self _setTextIndicator:dictionaryPopupInfo.textIndicator.get() fadeOut:NO];
        return [getLULookupDefinitionModuleClass() lookupAnimationControllerForTerm:dictionaryPopupInfo.attributedString.get() atLocation:textBaselineOrigin options:mutableOptions.get()];
    }

    return [getLULookupDefinitionModuleClass() lookupAnimationControllerForTerm:dictionaryPopupInfo.attributedString.get() atLocation:textBaselineOrigin options:dictionaryPopupInfo.options.get()];
}
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000

- (void)_setTextIndicator:(TextIndicator *)textIndicator fadeOut:(BOOL)fadeOut
{
    if (!textIndicator) {
        _private->textIndicatorWindow = nullptr;
        return;
    }

    if (!_private->textIndicatorWindow)
        _private->textIndicatorWindow = std::make_unique<TextIndicatorWindow>(self);

    NSRect textBoundingRectInWindowCoordinates = [self convertRect:[self _convertRectFromRootView:textIndicator->textBoundingRectInRootViewCoordinates()] toView:nil];
    NSRect textBoundingRectInScreenCoordinates = [self.window convertRectToScreen:textBoundingRectInWindowCoordinates];
    _private->textIndicatorWindow->setTextIndicator(textIndicator, NSRectToCGRect(textBoundingRectInScreenCoordinates), fadeOut);
}

- (void)_clearTextIndicator
{
    [self _setTextIndicator:nullptr fadeOut:NO];
}

- (void)_setTextIndicatorAnimationProgress:(float)progress
{
    if (_private->textIndicatorWindow)
        _private->textIndicatorWindow->setAnimationProgress(progress);
}

- (void)_showDictionaryLookupPopup:(const DictionaryPopupInfo&)dictionaryPopupInfo
{
    if (!dictionaryPopupInfo.attributedString)
        return;

    NSPoint textBaselineOrigin = dictionaryPopupInfo.origin;

    // Convert to screen coordinates.
    textBaselineOrigin = [self.window convertRectToScreen:NSMakeRect(textBaselineOrigin.x, textBaselineOrigin.y, 0, 0)].origin;

    if (canLoadLUTermOptionDisableSearchTermIndicator() && canLoadLUNotificationPopoverWillClose()) {
        if (!_private->hasInitializedLookupObserver) {
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_dictionaryLookupPopoverWillClose:) name:getLUNotificationPopoverWillClose() object:nil];
            _private->hasInitializedLookupObserver = YES;
        }

        RetainPtr<NSMutableDictionary> mutableOptions = adoptNS([dictionaryPopupInfo.options mutableCopy]);
        if (!mutableOptions)
            mutableOptions = adoptNS([[NSMutableDictionary alloc] init]);
        [mutableOptions setObject:@YES forKey:getLUTermOptionDisableSearchTermIndicator()];
        [self _setTextIndicator:dictionaryPopupInfo.textIndicator.get() fadeOut:NO];
        [getLULookupDefinitionModuleClass() showDefinitionForTerm:dictionaryPopupInfo.attributedString.get() atLocation:textBaselineOrigin options:mutableOptions.get()];
    } else
        [getLULookupDefinitionModuleClass() showDefinitionForTerm:dictionaryPopupInfo.attributedString.get() atLocation:textBaselineOrigin options:dictionaryPopupInfo.options.get()];
}

- (void)_dictionaryLookupPopoverWillClose:(NSNotification *)notification
{
    [self _setTextIndicator:nullptr fadeOut:NO];
}
#endif // PLATFORM(MAC)

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
- (WebMediaPlaybackTargetPicker *) _devicePicker
{
    if (!_private->m_playbackTargetPicker)
        _private->m_playbackTargetPicker = WebMediaPlaybackTargetPicker::create(*_private->page);

    return _private->m_playbackTargetPicker.get();
}

- (void)_showPlaybackTargetPicker:(const WebCore::IntPoint&)location hasVideo:(BOOL)hasVideo
{
    if (!_private->page)
        return;

    NSRect rectInWindowCoordinates = [self convertRect:[self _convertRectFromRootView:NSMakeRect(location.x(), location.y(), 0, 0)] toView:nil];
    NSRect rectInScreenCoordinates = [self.window convertRectToScreen:rectInWindowCoordinates];
    [self _devicePicker]->showPlaybackTargetPicker(rectInScreenCoordinates, hasVideo);
}

- (void)_startingMonitoringPlaybackTargets
{
    if (!_private->page)
        return;

    [self _devicePicker]->startingMonitoringPlaybackTargets();
}

- (void)_stopMonitoringPlaybackTargets
{
    if (!_private->page)
        return;

    [self _devicePicker]->stopMonitoringPlaybackTargets();
}
#endif

@end

@implementation WebView (WebViewDeviceOrientation)

- (void)_setDeviceOrientationProvider:(id<WebDeviceOrientationProvider>)deviceOrientationProvider
{
    if (_private)
        _private->m_deviceOrientationProvider = deviceOrientationProvider;
}

- (id<WebDeviceOrientationProvider>)_deviceOrientationProvider
{
    if (_private)
        return _private->m_deviceOrientationProvider;
    return nil;
}

@end

#if ENABLE(MEDIA_STREAM)
@implementation WebView (WebViewUserMedia)

- (void)_setUserMediaClient:(id<WebUserMediaClient>)userMediaClient
{
    if (_private)
        _private->m_userMediaClient = userMediaClient;
}

- (id<WebUserMediaClient>)_userMediaClient
{
    if (_private)
        return _private->m_userMediaClient;
    return nil;
}
@end
#endif

@implementation WebView (WebViewGeolocation)

- (void)_setGeolocationProvider:(id<WebGeolocationProvider>)geolocationProvider
{
    if (_private)
        _private->_geolocationProvider = geolocationProvider;
}

- (id<WebGeolocationProvider>)_geolocationProvider
{
    if (_private)
        return _private->_geolocationProvider;
    return nil;
}

- (void)_geolocationDidChangePosition:(WebGeolocationPosition *)position
{
#if ENABLE(GEOLOCATION)
    if (_private && _private->page)
        WebCore::GeolocationController::from(_private->page)->positionChanged(core(position));
#endif // ENABLE(GEOLOCATION)
}

- (void)_geolocationDidFailWithMessage:(NSString *)errorMessage
{
#if ENABLE(GEOLOCATION)
    if (_private && _private->page) {
        RefPtr<GeolocationError> geolocatioError = GeolocationError::create(GeolocationError::PositionUnavailable, errorMessage);
        WebCore::GeolocationController::from(_private->page)->errorOccurred(geolocatioError.get());
    }
#endif // ENABLE(GEOLOCATION)
}

#if PLATFORM(IOS)
- (void)_resetAllGeolocationPermission
{
#if ENABLE(GEOLOCATION)
    Frame* frame = [self _mainCoreFrame];
    if (frame)
        frame->resetAllGeolocationPermission();
#endif
}
#endif

@end

@implementation WebView (WebViewNotification)
- (void)_setNotificationProvider:(id<WebNotificationProvider>)notificationProvider
{
    if (_private && !_private->_notificationProvider) {
        _private->_notificationProvider = notificationProvider;
        [_private->_notificationProvider registerWebView:self];
    }
}

- (id<WebNotificationProvider>)_notificationProvider
{
    if (_private)
        return _private->_notificationProvider;
    return nil;
}

- (void)_notificationDidShow:(uint64_t)notificationID
{
    [[self _notificationProvider] webView:self didShowNotification:notificationID];
}

- (void)_notificationDidClick:(uint64_t)notificationID
{
    [[self _notificationProvider] webView:self didClickNotification:notificationID];
}

- (void)_notificationsDidClose:(NSArray *)notificationIDs
{
    [[self _notificationProvider] webView:self didCloseNotifications:notificationIDs];
}

- (uint64_t)_notificationIDForTesting:(JSValueRef)jsNotification
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    JSContextRef context = [[self mainFrame] globalContext];
    WebCore::Notification* notification = JSNotification::toWrapped(toJS(toJS(context), jsNotification));
    return static_cast<WebNotificationClient*>(NotificationController::clientFrom(_private->page))->notificationIDForTesting(notification);
#else
    return 0;
#endif
}
@end

#if PLATFORM(IOS)
@implementation WebView (WebViewIOSPDF)

+ (Class)_getPDFRepresentationClass
{
    if (s_pdfRepresentationClass)
        return s_pdfRepresentationClass;
    return [WebPDFView class]; // This is WebPDFRepresentation for PLATFORM(MAC).
}

+ (void)_setPDFRepresentationClass:(Class)pdfRepresentationClass
{
    s_pdfRepresentationClass = pdfRepresentationClass;
}

+ (Class)_getPDFViewClass
{
    if (s_pdfViewClass)
        return s_pdfViewClass;
    return [WebPDFView class];
}

+ (void)_setPDFViewClass:(Class)pdfViewClass
{
    s_pdfViewClass = pdfViewClass;
}

@end
#endif

@implementation WebView (WebViewFullScreen)

- (NSView*)fullScreenPlaceholderView
{
#if ENABLE(FULLSCREEN_API)
    if (_private->newFullscreenController && [_private->newFullscreenController isFullScreen])
        return [_private->newFullscreenController webViewPlaceholder];
#endif
    return nil;
}

@end

void WebInstallMemoryPressureHandler(void)
{
    MemoryPressureHandler::singleton().install();
}
