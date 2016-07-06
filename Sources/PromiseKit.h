#import <Foundation/NSObjCRuntime.h>
#import <PromiseKit/AnyPromise.h>
#import <Foundation/NSObject.h>
#import <Foundation/NSDate.h>
#import <dispatch/dispatch.h>

FOUNDATION_EXPORT double PromiseKitVersionNumber;
FOUNDATION_EXPORT const unsigned char PromiseKitVersionString[];

extern NSString * __nonnull const PMKErrorDomain;

#define PMKFailingPromiseIndexKey @"PMKFailingPromiseIndexKey"
#define PMKURLErrorFailingURLResponseKey @"PMKURLErrorFailingURLResponseKey"
#define PMKURLErrorFailingDataKey @"PMKURLErrorFailingDataKey"
#define PMKURLErrorFailingStringKey @"PMKURLErrorFailingStringKey"
#define PMKJSONErrorJSONObjectKey @"PMKJSONErrorJSONObjectKey"
#define PMKJoinPromisesKey @"PMKJoinPromisesKey"

#define PMKUnexpectedError 1l
#define PMKUnknownError 2l
#define PMKInvalidUsageError 3l
#define PMKAccessDeniedError 4l
#define PMKOperationCancelled 5l
#define PMKNotFoundError 6l
#define PMKJSONError 7l
#define PMKOperationFailed 8l
#define PMKTaskError 9l
#define PMKJoinError 10l

/**
 @return A new promise that resolves after the specified duration.

 @parameter duration The duration in seconds to wait before this promise is resolve.

 For example:

    PMKAfter(1).then(^{
        //…
    });
*/
extern AnyPromise * __nonnull PMKAfter(NSTimeInterval duration) NS_REFINED_FOR_SWIFT;



/**
 `when` is a mechanism for waiting more than one asynchronous task and responding when they are all complete.

 `PMKWhen` accepts varied input. If an array is passed then when those promises fulfill, when’s promise fulfills with an array of fulfillment values. If a dictionary is passed then the same occurs, but when’s promise fulfills with a dictionary of fulfillments keyed as per the input.

 Interestingly, if a single promise is passed then when waits on that single promise, and if a single non-promise object is passed then when fulfills immediately with that object. If the array or dictionary that is passed contains objects that are not promises, then these objects are considered fulfilled promises. The reason we do this is to allow a pattern know as "abstracting away asynchronicity".

 If *any* of the provided promises reject, the returned promise is immediately rejected with that promise’s rejection. The error’s `userInfo` object is supplemented with `PMKFailingPromiseIndexKey`.

 For example:

    PMKWhen(@[promise1, promise2]).then(^(NSArray *results){
        //…
    });

 @warning *Important* In the event of rejection the other promises will continue to resolve and as per any other promise will eithe fulfill or reject. This is the right pattern for `getter` style asynchronous tasks, but often for `setter` tasks (eg. storing data on a server), you most likely will need to wait on all tasks and then act based on which have succeeded and which have failed. In such situations use `PMKJoin`.

 @param input The input upon which to wait before resolving this promise.

 @return A promise that is resolved with either:

  1. An array of values from the provided array of promises.
  2. The value from the provided promise.
  3. The provided non-promise object.

 @see PMKJoin

*/
extern AnyPromise * __nonnull PMKWhen(id __nonnull input) NS_REFINED_FOR_SWIFT;



/**
 Creates a new promise that resolves only when all provided promises have resolved.

 Typically, you should use `PMKWhen`.

 For example:

    PMKJoin(@[promise1, promise2]).then(^(NSArray *resultingValues){
        //…
    }).catch(^(NSError *error){
        assert(error.domain == PMKErrorDomain);
        assert(error.code == PMKJoinError);

        NSArray *promises = error.userInfo[PMKJoinPromisesKey];
        for (AnyPromise *promise in promises) {
            if (promise.rejected) {
                //…
            }
        }
    });

 @param promises An array of promises.

 @return A promise that thens three parameters:

  1) An array of mixed values and errors from the resolved input.
  2) An array of values from the promises that fulfilled.
  3) An array of errors from the promises that rejected or nil if all promises fulfilled.

 @see when
*/
AnyPromise *__nonnull PMKJoin(NSArray * __nonnull promises) NS_REFINED_FOR_SWIFT;



/**
 Literally hangs this thread until the promise has resolved.
 
 Do not use hang… unless you are testing, playing or debugging.
 
 If you use it in production code I will literally and honestly cry like a child.
 
 @return The resolved value of the promise.

 @warning T SAFE. IT IS NOT SAFE. IT IS NOT SAFE. IT IS NOT SAFE. IT IS NO
*/
extern id __nullable PMKHang(AnyPromise * __nonnull promise);



/**
 Sets the unhandled exception handler.

 If an exception is thrown inside an AnyPromise handler it is caught and
 this handler is executed to determine if the promise is rejected.
 
 The default handler rejects the promise if an NSError or an NSString is
 thrown.
 
 The default handler in PromiseKit 1.x would reject whatever object was
 thrown (including nil).

 @warning *Important* This handler is provided to allow you to customize
 which exceptions cause rejection and which abort. You should either
 return a fully-formed NSError object or nil. Returning nil causes the
 exception to be re-thrown.

 @warning *Important* The handler is executed on an undefined queue.

 @warning *Important* This function is thread-safe, but to facilitate this
 it can only be called once per application lifetime and it must be called
 before any promise in the app throws an exception. Subsequent calls will
 silently fail.
*/
extern void PMKSetUnhandledExceptionHandler(NSError * __nullable (^__nonnull handler)(id __nullable));



/**
 Executes the provided block on a background queue.

 dispatch_promise is a convenient way to start a promise chain where the
 first step needs to run synchronously on a background queue.

    dispatch_promise(^{
        return md5(input);
    }).then(^(NSString *md5){
        NSLog(@"md5: %@", md5);
    });

 @param block The block to be executed in the background. Returning an `NSError` will reject the promise, everything else (including void) fulfills the promise.

 @return A promise resolved with the return value of the provided block.

 @see dispatch_async
*/
extern AnyPromise * __nonnull dispatch_promise(id __nonnull block) NS_SWIFT_UNAVAILABLE("Use our `DispatchQueue.async` override instead");



/**
 Executes the provided block on the specified background queue.

    dispatch_promise_on(myDispatchQueue, ^{
        return md5(input);
    }).then(^(NSString *md5){
        NSLog(@"md5: %@", md5);
    });

 @param block The block to be executed in the background. Returning an `NSError` will reject the promise, everything else (including void) fulfills the promise.

 @return A promise resolved with the return value of the provided block.

 @see dispatch_promise
*/
extern AnyPromise * __nonnull dispatch_promise_on(dispatch_queue_t __nonnull queue, id __nonnull block) NS_SWIFT_UNAVAILABLE("Use our `DispatchQueue.async` override instead");



#define PMKJSONDeserializationOptions ((NSJSONReadingOptions)(NSJSONReadingAllowFragments | NSJSONReadingMutableContainers))

/**
 Really we shouldn’t assume JSON for (application|text)/(x-)javascript,
 really we should return a String of Javascript. However in practice
 for the apps we write it *will be* JSON. Thus if you actually want
 a Javascript String, use the promise variant of our category functions.
*/
#define PMKHTTPURLResponseIsJSON(rsp) [@[@"application/json", @"text/json", @"text/javascript", @"application/x-javascript", @"application/javascript"] containsObject:[rsp MIMEType]]
#define PMKHTTPURLResponseIsImage(rsp) [@[@"image/tiff", @"image/jpeg", @"image/gif", @"image/png", @"image/ico", @"image/x-icon", @"image/bmp", @"image/x-bmp", @"image/x-xbitmap", @"image/x-win-bitmap"] containsObject:[rsp MIMEType]]
#define PMKHTTPURLResponseIsText(rsp) [[rsp MIMEType] hasPrefix:@"text/"]

/**
 The default queue for all calls to `then`, `catch` etc. is the main queue.

 By default this returns dispatch_get_main_queue()
 */
extern __nonnull dispatch_queue_t PMKDefaultDispatchQueue();

/**
 You may alter the default dispatch queue, but you may only alter it once, and you must alter it before any `then`, etc. calls are made in your app.
 
 The primary motivation for this function is so that your tests can operate off the main thread preventing dead-locking, or with `zalgo` to speed them up.
*/
extern void PMKSetDefaultDispatchQueue(__nonnull dispatch_queue_t);



#if defined(__has_include)
  #if __has_include(<PromiseKit/ACAccountStore+AnyPromise.h>)
    #import <PromiseKit/ACAccountStore+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/AVAudioSession+AnyPromise.h>)
    #import <PromiseKit/AVAudioSession+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/CKContainer+AnyPromise.h>)
    #import <PromiseKit/CKContainer+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/CKDatabase+AnyPromise.h>)
    #import <PromiseKit/CKDatabase+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/CLGeocoder+AnyPromise.h>)
    #import <PromiseKit/CLGeocoder+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/CLLocationManager+AnyPromise.h>)
    #import <PromiseKit/CLLocationManager+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/NSNotificationCenter+AnyPromise.h>)
    #import <PromiseKit/NSNotificationCenter+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/NSTask+AnyPromise.h>)
    #import <PromiseKit/NSTask+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/NSURLConnection+AnyPromise.h>)
    #import <PromiseKit/NSURLConnection+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/MKDirections+AnyPromise.h>)
    #import <PromiseKit/MKDirections+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/MKMapSnapshotter+AnyPromise.h>)
    #import <PromiseKit/MKMapSnapshotter+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/CALayer+AnyPromise.h>)
    #import <PromiseKit/CALayer+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/SLRequest+AnyPromise.h>)
    #import <PromiseKit/SLRequest+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/SKRequest+AnyPromise.h>)
    #import <PromiseKit/SKRequest+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/SCNetworkReachability+AnyPromise.h>)
    #import <PromiseKit/SCNetworkReachability+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/UIActionSheet+AnyPromise.h>)
    #import <PromiseKit/UIActionSheet+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/UIAlertView+AnyPromise.h>)
    #import <PromiseKit/UIAlertView+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/UIView+AnyPromise.h>)
    #import <PromiseKit/UIView+AnyPromise.h>
  #endif
  #if __has_include(<PromiseKit/UIViewController+AnyPromise.h>)
    #import <PromiseKit/UIViewController+AnyPromise.h>
  #endif
#endif


#if !defined(SWIFT_PASTE)
# define SWIFT_PASTE_HELPER(x, y) x##y
# define SWIFT_PASTE(x, y) SWIFT_PASTE_HELPER(x, y)
#endif

#if !defined(SWIFT_EXTENSION)
# define SWIFT_EXTENSION(M) SWIFT_PASTE(M##_Swift_, __LINE__)
#endif

@interface NSError (SWIFT_EXTENSION(PromiseKit))
+ (NSError * __nonnull)cancelledError;
+ (void)registerCancelledErrorDomain:(NSString * __nonnull)domain code:(NSInteger)code;
@property (nonatomic, readonly) BOOL isCancelled;
@end

#undef SWIFT_PASTE_HELPER
#undef SWIFT_PASTE
#undef SWIFT_EXTENSION
