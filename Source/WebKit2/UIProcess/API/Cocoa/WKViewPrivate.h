/*
 * Copyright (C) 2011, 2014 Apple Inc. All rights reserved.
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

#import <WebKit/WKActionMenuTypes.h>
#import <WebKit/WKBase.h>
#import <WebKit/WKImmediateActionTypes.h>
#import <WebKit/WKView.h>

@interface WKView (Private)

/* C SPI support. */

@property (readonly) WKPageRef pageRef;

#if TARGET_OS_IPHONE
- (id)initWithFrame:(CGRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef;
- (id)initWithFrame:(CGRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef relatedToPage:(WKPageRef)relatedPage;
#else
- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef;
- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef relatedToPage:(WKPageRef)relatedPage;
- (id)initWithFrame:(NSRect)frame configurationRef:(WKPageConfigurationRef)configuration;
#endif

#if TARGET_OS_IPHONE

@property (nonatomic) CGSize minimumLayoutSizeOverride;

// Define the inset of the scrollview unusable by the web page.
@property (nonatomic, setter=_setObscuredInsets:) UIEdgeInsets _obscuredInsets;

@property (nonatomic, setter=_setBackgroundExtendsBeyondPage:) BOOL _backgroundExtendsBeyondPage;

// This is deprecated and should be removed entirely: <rdar://problem/16294704>.
@property (readonly) UIColor *_pageExtendedBackgroundColor;

- (void)_beginInteractiveObscuredInsetsChange;
- (void)_endInteractiveObscuredInsetsChange;
- (void)_didRelaunchProcess;

#else

- (NSPrintOperation *)printOperationWithPrintInfo:(NSPrintInfo *)printInfo forFrame:(WKFrameRef)frameRef;
- (BOOL)canChangeFrameLayout:(WKFrameRef)frameRef;

- (void)setFrame:(NSRect)rect andScrollBy:(NSSize)offset;

// Stops updating the size of the page as the WKView frame size updates.
// This should always be followed by enableFrameSizeUpdates. Calls can be nested.
- (void)disableFrameSizeUpdates;
// Immediately updates the size of the page to match WKView's frame size
// and allows subsequent updates as the frame size is set. Calls can be nested.
- (void)enableFrameSizeUpdates;
- (BOOL)frameSizeUpdatesDisabled;

+ (void)hideWordDefinitionWindow;

@property (readwrite) CGFloat minimumLayoutWidth;
@property (readwrite) CGFloat minimumWidthForAutoLayout;
@property (readwrite) NSSize minimumSizeForAutoLayout;
@property (readwrite) BOOL shouldClipToVisibleRect;
@property (readwrite) BOOL shouldExpandToViewHeightForAutoLayout;
@property (readonly, getter=isUsingUISideCompositing) BOOL usingUISideCompositing;
@property (readwrite) BOOL allowsMagnification;
@property (readwrite) double magnification;
@property (readwrite, setter=_setIgnoresNonWheelEvents:) BOOL _ignoresNonWheelEvents;
@property (readwrite, setter=_setIgnoresAllEvents:) BOOL _ignoresAllEvents;
@property (readwrite) BOOL allowsBackForwardNavigationGestures;
@property (nonatomic, setter=_setTopContentInset:) CGFloat _topContentInset;
@property (nonatomic, setter=_setTotalHeightOfBanners:) CGFloat _totalHeightOfBanners;

@property (nonatomic, setter=_setOverrideDeviceScaleFactor:) CGFloat _overrideDeviceScaleFactor WK_AVAILABLE(WK_MAC_TBA, NA);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
@property (nonatomic, setter=_setAutomaticallyAdjustsContentInsets:) BOOL _automaticallyAdjustsContentInsets;
#endif

@property (readonly) NSColor *_pageExtendedBackgroundColor;
@property (copy, nonatomic) NSColor *underlayColor;

#if WK_API_ENABLED
@property (strong, nonatomic, setter=_setInspectorAttachmentView:) NSView *_inspectorAttachmentView WK_AVAILABLE(WK_MAC_TBA, NA);
#endif

- (NSView*)fullScreenPlaceholderView;
- (NSWindow*)createFullScreenWindow;

- (void)beginDeferringViewInWindowChanges;
- (void)endDeferringViewInWindowChanges;
- (void)endDeferringViewInWindowChangesSync;
- (BOOL)isDeferringViewInWindowChanges;
- (void)_prepareForMoveToWindow:(NSWindow *)targetWindow withCompletionHandler:(void(^)(void))completionHandler;

- (BOOL)windowOcclusionDetectionEnabled;
- (void)setWindowOcclusionDetectionEnabled:(BOOL)flag;

- (void)forceAsyncDrawingAreaSizeUpdate:(NSSize)size;
- (void)waitForAsyncDrawingAreaSizeUpdate;

- (void)setMagnification:(double)magnification centeredAtPoint:(NSPoint)point;

- (void)saveBackForwardSnapshotForCurrentItem;
- (void)saveBackForwardSnapshotForItem:(WKBackForwardListItemRef)item;

// Views must be layer-backed, have no transform applied, be in back-to-front z-order, and the whole set must be a contiguous opaque rectangle.
- (void)_setCustomSwipeViews:(NSArray *)customSwipeViews;
// The top content inset is applied in the window's coordinate space, to the union of the custom swipe view's frames.
- (void)_setCustomSwipeViewsTopContentInset:(float)topContentInset;
- (BOOL)_tryToSwipeWithEvent:(NSEvent *)event ignoringPinnedState:(BOOL)ignoringPinnedState;
// The rect returned is always that of the snapshot, and only if it is the view being manipulated by the swipe. This only works for layer-backed windows.
- (void)_setDidMoveSwipeSnapshotCallback:(void(^)(CGRect swipeSnapshotRectInWindowCoordinates))callback;

- (NSArray *)_actionMenuItemsForHitTestResult:(WKHitTestResultRef)hitTestResult withType:(_WKActionMenuType)type defaultActionMenuItems:(NSArray *)defaultMenuItems;
- (NSArray *)_actionMenuItemsForHitTestResult:(WKHitTestResultRef)hitTestResult withType:(_WKActionMenuType)type defaultActionMenuItems:(NSArray *)defaultMenuItems userData:(WKTypeRef)userData;

// Clients that want to maintain default behavior can return nil. To disable the immediate action entirely, return NSNull. And to
// do something custom, return an object that conforms to the NSImmediateActionAnimationController protocol.
- (id)_immediateActionAnimationControllerForHitTestResult:(WKHitTestResultRef)hitTestResult withType:(_WKImmediateActionType)type userData:(WKTypeRef)userData;

- (void)_prepareForImmediateActionAnimation;
- (void)_cancelImmediateActionAnimation;
- (void)_completeImmediateActionAnimation;

- (void)_dismissContentRelativeChildWindows;

- (void)_didChangeContentSize:(NSSize)newSize;
#endif

@end
