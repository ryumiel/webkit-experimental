/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WKImmediateActionController.h"

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000

#import "WKNSURLExtras.h"
#import "WKViewInternal.h"
#import "WebPageMessages.h"
#import "WebPageProxy.h"
#import "WebPageProxyMessages.h"
#import "WebProcessProxy.h"
#import <WebCore/DataDetectorsSPI.h>
#import <WebCore/GeometryUtilities.h>
#import <WebCore/LookupSPI.h>
#import <WebCore/NSMenuSPI.h>
#import <WebCore/NSPopoverSPI.h>
#import <WebCore/QuickLookMacSPI.h>
#import <WebCore/SoftLinking.h>
#import <WebCore/URL.h>

SOFT_LINK_FRAMEWORK_IN_UMBRELLA(Quartz, QuickLookUI)
SOFT_LINK_CLASS(QuickLookUI, QLPreviewMenuItem)
SOFT_LINK_CONSTANT_MAY_FAIL(Lookup, LUTermOptionDisableSearchTermIndicator, NSString *)

using namespace WebCore;
using namespace WebKit;

@interface WKImmediateActionController () <QLPreviewMenuItemDelegate>
@end

@interface WebAnimationController : NSObject <NSImmediateActionAnimationController> {
}
@end

@implementation WebAnimationController
@end

@implementation WKImmediateActionController

- (instancetype)initWithPage:(WebPageProxy&)page view:(WKView *)wkView recognizer:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    self = [super init];

    if (!self)
        return nil;

    _page = &page;
    _wkView = wkView;
    _type = kWKImmediateActionNone;
    _immediateActionRecognizer = immediateActionRecognizer;
    _hasActiveImmediateAction = NO;

    return self;
}

- (void)willDestroyView:(WKView *)view
{
    _page = nullptr;
    _wkView = nil;
    _hitTestResult = ActionMenuHitTestResult();

    id animationController = [_immediateActionRecognizer animationController];
    if ([animationController isKindOfClass:NSClassFromString(@"QLPreviewMenuItem")]) {
        QLPreviewMenuItem *menuItem = (QLPreviewMenuItem *)animationController;
        menuItem.delegate = nil;
    }

    _immediateActionRecognizer = nil;
    _currentActionContext = nil;
    _hasActiveImmediateAction = NO;
}

- (void)_cancelImmediateAction
{
    // Reset the recognizer by turning it off and on again.
    [_immediateActionRecognizer setEnabled:NO];
    [_immediateActionRecognizer setEnabled:YES];

    [self _clearImmediateActionState];
}

- (void)_cancelImmediateActionIfNeeded
{
    if (![_immediateActionRecognizer animationController])
        [self _cancelImmediateAction];
}

- (void)_clearImmediateActionState
{
    if (_page)
        _page->clearTextIndicator();

    if (_currentActionContext && _hasActivatedActionContext) {
        _hasActivatedActionContext = NO;
        [getDDActionsManagerClass() didUseActions];
    }

    _state = ImmediateActionState::None;
    _hitTestResult = ActionMenuHitTestResult();
    _type = kWKImmediateActionNone;
    _currentActionContext = nil;
    _userData = nil;
    _currentQLPreviewMenuItem = nil;
    _hasActiveImmediateAction = NO;
}

- (void)didPerformActionMenuHitTest:(const ActionMenuHitTestResult&)hitTestResult userData:(API::Object*)userData
{
    // If we've already given up on this gesture (either because it was canceled or the
    // willBeginAnimation timeout expired), we shouldn't build a new animationController for it.
    if (_state != ImmediateActionState::Pending)
        return;

    // FIXME: This needs to use the WebKit2 callback mechanism to avoid out-of-order replies.
    _state = ImmediateActionState::Ready;
    _hitTestResult = hitTestResult;
    _userData = userData;

    [self _updateImmediateActionItem];
    [self _cancelImmediateActionIfNeeded];
}

- (void)dismissContentRelativeChildWindows
{
    _page->setMaintainsInactiveSelection(false);
    [_currentQLPreviewMenuItem close];
}

- (BOOL)hasActiveImmediateAction
{
    return _hasActiveImmediateAction;
}

#pragma mark NSImmediateActionGestureRecognizerDelegate

- (void)immediateActionRecognizerWillPrepare:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    if (immediateActionRecognizer != _immediateActionRecognizer)
        return;

    [_wkView _prepareForImmediateActionAnimation];

    [_wkView _dismissContentRelativeChildWindows];

    _page->setMaintainsInactiveSelection(true);

    _page->performActionMenuHitTestAtLocation([immediateActionRecognizer locationInView:immediateActionRecognizer.view], true);

    _state = ImmediateActionState::Pending;
    immediateActionRecognizer.animationController = nil;
}

- (void)immediateActionRecognizerWillBeginAnimation:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    if (immediateActionRecognizer != _immediateActionRecognizer)
        return;

    if (_state == ImmediateActionState::None)
        return;

    _hasActiveImmediateAction = YES;

    // FIXME: We need to be able to cancel this if the gesture recognizer is cancelled.
    // FIXME: Connection can be null if the process is closed; we should clean up better in that case.
    if (_state == ImmediateActionState::Pending) {
        if (auto* connection = _page->process().connection()) {
            bool receivedReply = connection->waitForAndDispatchImmediately<Messages::WebPageProxy::DidPerformActionMenuHitTest>(_page->pageID(), std::chrono::milliseconds(500));
            if (!receivedReply)
                _state = ImmediateActionState::TimedOut;
        }
    }

    if (_state != ImmediateActionState::Ready) {
        [self _updateImmediateActionItem];
        [self _cancelImmediateActionIfNeeded];
    }

    if (_currentActionContext) {
        _hasActivatedActionContext = YES;
        if (![getDDActionsManagerClass() shouldUseActionsWithContext:_currentActionContext.get()])
            [self _cancelImmediateAction];
    }
}

- (void)immediateActionRecognizerDidUpdateAnimation:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    if (immediateActionRecognizer != _immediateActionRecognizer)
        return;

    _page->immediateActionDidUpdate([immediateActionRecognizer animationProgress]);
    if (_hitTestResult.contentPreventsDefault)
        return;

    _page->setTextIndicatorAnimationProgress([immediateActionRecognizer animationProgress]);
}

- (void)immediateActionRecognizerDidCancelAnimation:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    if (immediateActionRecognizer != _immediateActionRecognizer)
        return;

    _page->immediateActionDidCancel();

    [_wkView _cancelImmediateActionAnimation];

    _page->setTextIndicatorAnimationProgress(0);
    [self _clearImmediateActionState];
    _page->setMaintainsInactiveSelection(false);
}

- (void)immediateActionRecognizerDidCompleteAnimation:(NSImmediateActionGestureRecognizer *)immediateActionRecognizer
{
    if (immediateActionRecognizer != _immediateActionRecognizer)
        return;

    _page->immediateActionDidComplete();

    [_wkView _completeImmediateActionAnimation];

    _page->setTextIndicatorAnimationProgress(1);
}

- (PassRefPtr<WebHitTestResult>)_webHitTestResult
{
    RefPtr<WebHitTestResult> hitTestResult;
    if (_state == ImmediateActionState::Ready)
        hitTestResult = WebHitTestResult::create(_hitTestResult.hitTestResult);
    else
        hitTestResult = _page->lastMouseMoveHitTestResult();

    return hitTestResult.release();
}

#pragma mark Immediate actions

- (id <NSImmediateActionAnimationController>)_defaultAnimationController
{
    if (_hitTestResult.contentPreventsDefault) {
        RetainPtr<WebAnimationController> dummyController = [[WebAnimationController alloc] init];
        return dummyController.get();
    }

    RefPtr<WebHitTestResult> hitTestResult = [self _webHitTestResult];

    if (!hitTestResult)
        return nil;

    String absoluteLinkURL = hitTestResult->absoluteLinkURL();
    if (!absoluteLinkURL.isEmpty() && WebCore::protocolIsInHTTPFamily(absoluteLinkURL)) {
        _type = kWKImmediateActionLinkPreview;

        if (TextIndicator *textIndicator = _hitTestResult.linkTextIndicator.get())
            _page->setTextIndicator(textIndicator->data(), false);

        RetainPtr<QLPreviewMenuItem> qlPreviewLinkItem = [NSMenuItem standardQuickLookMenuItem];
        [qlPreviewLinkItem setPreviewStyle:QLPreviewStylePopover];
        [qlPreviewLinkItem setDelegate:self];
        _currentQLPreviewMenuItem = qlPreviewLinkItem.get();
        return (id<NSImmediateActionAnimationController>)qlPreviewLinkItem.get();
    }

    if (hitTestResult->isTextNode() || hitTestResult->isOverTextInsideFormControlElement()) {
        if (NSMenuItem *immediateActionItem = [self _menuItemForDataDetectedText]) {
            _type = kWKImmediateActionDataDetectedItem;
            return (id<NSImmediateActionAnimationController>)immediateActionItem;
        }

        if (id<NSImmediateActionAnimationController> textAnimationController = [self _animationControllerForText]) {
            _type = kWKImmediateActionLookupText;
            return textAnimationController;
        }
    }

    return nil;
}

- (void)_updateImmediateActionItem
{
    _type = kWKImmediateActionNone;

    id <NSImmediateActionAnimationController> defaultAnimationController = [self _defaultAnimationController];

    if (_hitTestResult.contentPreventsDefault) {
        [_immediateActionRecognizer.get() setAnimationController:defaultAnimationController];
        return;
    }

    RefPtr<WebHitTestResult> hitTestResult = [self _webHitTestResult];
    id customClientAnimationController = [_wkView _immediateActionAnimationControllerForHitTestResult:toAPI(hitTestResult.get()) withType:_type userData:toAPI(_userData.get())];

    if (customClientAnimationController == [NSNull null]) {
        [self _cancelImmediateAction];
        return;
    }

    if (customClientAnimationController && [customClientAnimationController conformsToProtocol:@protocol(NSImmediateActionAnimationController)])
        [_immediateActionRecognizer setAnimationController:(id <NSImmediateActionAnimationController>)customClientAnimationController];
    else
        [_immediateActionRecognizer setAnimationController:defaultAnimationController];
}

#pragma mark QLPreviewMenuItemDelegate implementation

- (NSView *)menuItem:(NSMenuItem *)menuItem viewAtScreenPoint:(NSPoint)screenPoint
{
    return _wkView;
}

- (id<QLPreviewItem>)menuItem:(NSMenuItem *)menuItem previewItemAtPoint:(NSPoint)point
{
    if (!_wkView)
        return nil;

    RefPtr<WebHitTestResult> hitTestResult = [self _webHitTestResult];
    return [NSURL _web_URLWithWTFString:hitTestResult->absoluteLinkURL()];
}

- (NSRectEdge)menuItem:(NSMenuItem *)menuItem preferredEdgeForPoint:(NSPoint)point
{
    return NSMaxYEdge;
}

- (void)menuItemDidClose:(NSMenuItem *)menuItem
{
    [self _clearImmediateActionState];
}

- (NSRect)menuItem:(NSMenuItem *)menuItem itemFrameForPoint:(NSPoint)point
{
    if (!_wkView)
        return NSZeroRect;

    RefPtr<WebHitTestResult> hitTestResult = [self _webHitTestResult];
    return [_wkView convertRect:hitTestResult->elementBoundingBox() toView:nil];
}

- (NSSize)menuItem:(NSMenuItem *)menuItem maxSizeForPoint:(NSPoint)point
{
    if (!_wkView)
        return NSZeroSize;

    NSSize screenSize = _wkView.window.screen.frame.size;
    FloatRect largestRect = largestRectWithAspectRatioInsideRect(screenSize.width / screenSize.height, _wkView.bounds);
    return NSMakeSize(largestRect.width() * 0.75, largestRect.height() * 0.75);
}

#pragma mark Data Detectors actions

- (NSMenuItem *)_menuItemForDataDetectedText
{
    DDActionContext *actionContext = _hitTestResult.actionContext.get();
    if (!actionContext)
        return nil;

    actionContext.altMode = YES;
    actionContext.immediate = YES;
    if ([[getDDActionsManagerClass() sharedManager] respondsToSelector:@selector(hasActionsForResult:actionContext:)]) {
        if (![[getDDActionsManagerClass() sharedManager] hasActionsForResult:actionContext.mainResult actionContext:actionContext])
            return nil;
    }

    RefPtr<WebPageProxy> page = _page;
    PageOverlay::PageOverlayID overlayID = _hitTestResult.detectedDataOriginatingPageOverlay;
    _currentActionContext = [actionContext contextForView:_wkView altMode:YES interactionStartedHandler:^() {
        page->send(Messages::WebPage::DataDetectorsDidPresentUI(overlayID));
    } interactionChangedHandler:^() {
        if (_hitTestResult.detectedDataTextIndicator)
            page->setTextIndicator(_hitTestResult.detectedDataTextIndicator->data(), false);
        page->send(Messages::WebPage::DataDetectorsDidChangeUI(overlayID));
    } interactionStoppedHandler:^() {
        page->send(Messages::WebPage::DataDetectorsDidHideUI(overlayID));
        [self _clearImmediateActionState];
    }];

    [_currentActionContext setHighlightFrame:[_wkView.window convertRectToScreen:[_wkView convertRect:_hitTestResult.detectedDataBoundingBox toView:nil]]];

    NSArray *menuItems = [[getDDActionsManagerClass() sharedManager] menuItemsForResult:[_currentActionContext mainResult] actionContext:_currentActionContext.get()];

    if (menuItems.count != 1)
        return nil;

    return menuItems.lastObject;
}

#pragma mark Text action

- (id<NSImmediateActionAnimationController>)_animationControllerForText
{
    if (_state != ImmediateActionState::Ready)
        return nil;

    if (!getLULookupDefinitionModuleClass())
        return nil;

    DictionaryPopupInfo dictionaryPopupInfo = _hitTestResult.dictionaryPopupInfo;
    if (!dictionaryPopupInfo.attributedString.string)
        return nil;

    // Convert baseline to screen coordinates.
    NSPoint textBaselineOrigin = dictionaryPopupInfo.origin;
    textBaselineOrigin = [_wkView convertPoint:textBaselineOrigin toView:nil];
    textBaselineOrigin = [_wkView.window convertRectToScreen:NSMakeRect(textBaselineOrigin.x, textBaselineOrigin.y, 0, 0)].origin;

    RetainPtr<NSMutableDictionary> mutableOptions = adoptNS([(NSDictionary *)dictionaryPopupInfo.options.get() mutableCopy]);
    if (canLoadLUTermOptionDisableSearchTermIndicator() && dictionaryPopupInfo.textIndicator.contentImage) {
        [_wkView _setTextIndicator:TextIndicator::create(dictionaryPopupInfo.textIndicator) fadeOut:NO];
        [mutableOptions setObject:@YES forKey:getLUTermOptionDisableSearchTermIndicator()];
        return [getLULookupDefinitionModuleClass() lookupAnimationControllerForTerm:dictionaryPopupInfo.attributedString.string.get() atLocation:textBaselineOrigin options:mutableOptions.get()];
    }
    return [getLULookupDefinitionModuleClass() lookupAnimationControllerForTerm:dictionaryPopupInfo.attributedString.string.get() atLocation:textBaselineOrigin options:mutableOptions.get()];
}

@end

#endif // PLATFORM(MAC)
