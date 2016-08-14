//
//  BSHTTPCookieStorage.m
//
//  Created by Sasmito Adibowo on 02-07-12.
//  Copyright (c) 2012 Basil Salad Software. All rights reserved.
//  http://basilsalad.com
//
//  Licensed under the BSD License <http://www.opensource.org/licenses/bsd-license>
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// this is ARC code.
#if !__has_feature(objc_arc)
#error Need automatic reference counting to compile this.
#endif

#import "BSHTTPCookieStorage.h"


@interface BSHTTPCookieStorage()

/*
 Cookie storage is stored in the order of
 domain -> path -> name

 This one stores cookies that are subdomain specific
 */
@property (nonatomic,strong,readonly) NSMutableDictionary* subdomainCookies;

/*
 Cookie storage is stored in the order of
 domain -> path -> name

 This one stores cookies global for a domain.
 */
@property (nonatomic,strong,readonly) NSMutableDictionary* domainGlobalCookies;

@end


@implementation BSHTTPCookieStorage

@synthesize subdomainCookies = _subdomainCookies;
@synthesize domainGlobalCookies = _domainGlobalCookies;


-(void) removeCookiesNamed:(NSString*) cookieName
{
    void (^removeCookiesInStorage)(NSMutableDictionary*) = ^(NSMutableDictionary* domainStorage) {
        [domainStorage enumerateKeysAndObjectsWithOptions:NSEnumerationConcurrent usingBlock:^(id domain, NSMutableDictionary* pathStorage, BOOL *stop) {
            [pathStorage enumerateKeysAndObjectsWithOptions:NSEnumerationConcurrent usingBlock:^(id path, NSMutableDictionary* nameStorage, BOOL *stop) {
                [nameStorage removeObjectForKey:cookieName];
            }];
        }];
    };

    removeCookiesInStorage(self.domainGlobalCookies);
    removeCookiesInStorage(self.subdomainCookies);
}


- (void)setCookie:(NSHTTPCookie *)aCookie
{
    // only domain names are case insensitive
    NSString* domain = [[aCookie domain] lowercaseString];
    NSString* path = [aCookie path];
    NSString* name = [aCookie name];

    NSMutableDictionary* domainStorage = [domain hasPrefix:@"."] ? self.domainGlobalCookies : self.subdomainCookies;

    NSMutableDictionary* pathStorage = [domainStorage objectForKey:domain];
    if (!pathStorage) {
        pathStorage = [NSMutableDictionary new];
        [domainStorage setObject:pathStorage forKey:domain];
    }
    NSMutableDictionary* nameStorage = [pathStorage objectForKey:path];
    if (!nameStorage) {
        nameStorage = [NSMutableDictionary new];
        [pathStorage setObject:nameStorage forKey:path];
    }

    if ([[aCookie expiresDate] timeIntervalSinceNow] < 0) {
        // delete cookie
        [nameStorage removeObjectForKey:name];
    } else {
        [nameStorage setObject:aCookie forKey:name];
    }
}


-(void) updateFromCookieStorage:(BSHTTPCookieStorage*) other
{
    void(^updateCookies)(NSDictionary*) = ^(NSDictionary* domainStorage) {
        [domainStorage enumerateKeysAndObjectsUsingBlock:^(NSString* domain, NSDictionary* pathStorage, BOOL *stop) {
            [pathStorage enumerateKeysAndObjectsUsingBlock:^(NSString* path, NSDictionary* nameStorage, BOOL *stop) {
                [nameStorage enumerateKeysAndObjectsUsingBlock:^(NSString* name, NSHTTPCookie* cookie, BOOL *stop) {
                    [self setCookie:cookie];
                }];
            }];
        }];
    };

    updateCookies(other.domainGlobalCookies);
    updateCookies(other.subdomainCookies);
}


- (NSArray *)cookiesForURL:(NSURL *)theURL
{
    NSMutableDictionary* resultCookies = [NSMutableDictionary new];
    NSString* cookiePath = [theURL path];

    void (^cookieFinder)(NSString*,NSDictionary*) = ^(NSString* domainKey,NSDictionary* domainStorage) {
        NSMutableDictionary* pathStorage = [domainStorage objectForKey:domainKey];
        if (!pathStorage) {
            return;
        }
        for (NSString* path in pathStorage) {
            if ([path isEqualToString:@"/"] || [cookiePath hasPrefix:path]) {
                NSMutableDictionary* nameStorage = [pathStorage objectForKey:path];
                [resultCookies addEntriesFromDictionary:nameStorage];
            }
        }
    };

    NSString* cookieDomain = [[theURL host] lowercaseString];

    // find domain-global cookies
    NSRange range = [cookieDomain rangeOfString:@"."];
    if (range.location != NSNotFound) {
        NSString* globalDomain = [cookieDomain substringFromIndex:range.location];
        cookieFinder(globalDomain,self.domainGlobalCookies);
        cookieFinder([NSString stringWithFormat:@".%@", cookieDomain], self.domainGlobalCookies);
    }

    // subdomain cookies will override the domain-global ones.
    cookieFinder(cookieDomain,self.subdomainCookies);

    return [resultCookies allValues];
}


-(void) loadCookies:(id<NSFastEnumeration>) cookies
{
    for (NSHTTPCookie* cookie in cookies) {
        [self setCookie:cookie];
    }
}


-(void) handleCookiesInRequest:(NSMutableURLRequest*) request
{
    NSURL* url = request.URL;
    NSArray* cookies = [self cookiesForURL:url];
    NSDictionary* headers = [NSHTTPCookie requestHeaderFieldsWithCookies:cookies];

    NSUInteger count = [headers count];
    __unsafe_unretained id keys[count], values[count];
    [headers getObjects:values andKeys:keys];

    for (NSUInteger i=0;i<count;i++) {
        [request setValue:values[i] forHTTPHeaderField:keys[i]];
    }
}



-(void) handleCookiesInResponse:(NSHTTPURLResponse*) response
{
    NSURL* url = response.URL;
    NSArray* cookies = [NSHTTPCookie cookiesWithResponseHeaderFields:response.allHeaderFields forURL:url];
    [self loadCookies:cookies];
}



-(void) handleWebScriptCookies:(NSString*) jsCookiesString forURLString:(NSString*) urlString
{
    if (![jsCookiesString isKindOfClass:[NSString class]]) {
        NSLog(@"Not a valid cookie string: %@",jsCookiesString);
        return;
    }
    if (![urlString isKindOfClass:[NSString class]]) {
        NSLog(@"Not a URL string: %@",urlString);
        return;
    }

    if ([@"undefined" isEqualToString:jsCookiesString] || [@"null" isEqualToString:jsCookiesString]) {
        NSLog(@"Invalid cookie string");
        return;
    }
    NSURL* jsUrl = [NSURL URLWithString:urlString];
    if (!jsUrl) {
        NSLog(@"Malformed URL String: %@",urlString);
        return;
    }

    NSString* const urlDomain = [jsUrl host];
    if (!urlDomain) {
        NSLog(@"No domain in URL %@ - ignoring.",urlDomain);
        return;
    }

    NSArray* cookiesArray = [jsCookiesString componentsSeparatedByString:@";"];
    NSMutableDictionary* cookiesDict = [NSMutableDictionary dictionaryWithCapacity:cookiesArray.count];
    NSCharacterSet* whitespace = [NSCharacterSet whitespaceCharacterSet];
    for (NSString* cookiePair in cookiesArray) {
        NSArray* pair = [cookiePair componentsSeparatedByString:@"="];
        if (pair.count == 2) {
            NSString* key = [[pair objectAtIndex:0] stringByTrimmingCharactersInSet:whitespace];
            NSString* value = [[pair objectAtIndex:1] stringByTrimmingCharactersInSet:whitespace];
            if (key.length > 0 && value.length > 0) {
                // don't decode the cookie values
                [cookiesDict setValue:value forKey:key];
            }
        }
    }

    // five years expiry for JavaScript cookies.
    // why five years? Because it looks like Yammer's JavaScript-based cookies usually expires in five years.
    NSDate* const cookieExpiryDate = [NSDate dateWithTimeIntervalSinceNow:5 * 365.25f * 24 * 3600];

    // we got all  JavaScript's cookie name/value pairs in 'cokiesDict' Now find existing cookies and override their values.
    NSArray* existingCookies = [self cookiesForURL:jsUrl];
    for (NSHTTPCookie* existingCookie in existingCookies) {
        NSString* existingName = existingCookie.name;
        NSString* updatedValue = [cookiesDict objectForKey:existingName];
        if (updatedValue) {
            if (![updatedValue isEqualToString:existingCookie.value]) {
                // override
                NSMutableDictionary* cookieProperties = [NSMutableDictionary dictionaryWithDictionary:existingCookie.properties];
                [cookieProperties setObject:updatedValue forKey:NSHTTPCookieValue];
                [cookieProperties setObject:cookieExpiryDate forKey:NSHTTPCookieExpires];
                NSHTTPCookie* updatedCookie = [NSHTTPCookie cookieWithProperties:cookieProperties];
                [self setCookie:updatedCookie];
            }

            // we already found an existing one, don't add it as domain global cookie
            [cookiesDict removeObjectForKey:existingName];
        }
    }

    // now set the rest as domain-global cookies
    if (cookiesDict.count > 0) {
        NSString* cookieDomain = urlDomain;
        NSArray* domainComponents = [urlDomain componentsSeparatedByString:@"."];
        if (domainComponents.count > 2) {
            NSMutableString* makeDomain = [NSMutableString stringWithCapacity:cookieDomain.length];
            [domainComponents enumerateObjectsUsingBlock:^(NSString* component, NSUInteger idx, BOOL *stop) {
                if (idx == 0) {
                    return; // skip the first one
                }
                [makeDomain appendFormat:@".%@",component];
            }];
            cookieDomain = makeDomain;
        }

        NSString* const cookiePath = @"/";
        [cookiesDict enumerateKeysAndObjectsUsingBlock:^(NSString* cookieName, NSString* cookieValue, BOOL *stop) {
            NSMutableDictionary* cookieParams = [NSMutableDictionary dictionaryWithCapacity:6];
            [cookieParams setObject:cookieDomain forKey:NSHTTPCookieDomain];
            [cookieParams setObject:cookiePath forKey:NSHTTPCookiePath];
            [cookieParams setObject:cookieExpiryDate forKey:NSHTTPCookieExpires];

            [cookieParams setObject:cookieName forKey:NSHTTPCookieName];
            [cookieParams setObject:cookieValue forKey:NSHTTPCookieValue];

            NSHTTPCookie* cookie = [NSHTTPCookie cookieWithProperties:cookieParams];
            [self setCookie:cookie];
        }];
    }
}


#pragma mark Property Access

-(NSMutableDictionary *)subdomainCookies
{
    if (!_subdomainCookies) {
        _subdomainCookies = [NSMutableDictionary new];
    }
    return _subdomainCookies;
}


-(NSMutableDictionary *)domainGlobalCookies
{
    if (!_domainGlobalCookies) {
        _domainGlobalCookies = [NSMutableDictionary new];
    }
    return _domainGlobalCookies;
}


-(void)reset
{
    [self.subdomainCookies removeAllObjects];
    [self.domainGlobalCookies removeAllObjects];
}


#pragma mark NSCoding

-(id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [self init]) {
        _domainGlobalCookies = [aDecoder decodeObjectForKey:@"domainGlobalCookies"];
        _subdomainCookies = [aDecoder decodeObjectForKey:@"subdomainCookies"];
    }
    return self;
}

- (NSDictionary *)removeSessionCookies:(NSDictionary *)cookies {
    NSMutableDictionary *outCookies = [[NSMutableDictionary alloc] init];

    // ugh, so many nested dicts
    for (NSString *domain in cookies) {
        NSDictionary *domainCookies = [cookies objectForKey:domain];
        NSMutableDictionary *outDomainCookies = [[NSMutableDictionary alloc] init];
        [outCookies setObject:outDomainCookies forKey:domain];
        for (NSString *path in domainCookies) {
            NSDictionary *pathCookies = [domainCookies objectForKey:path];
            NSMutableDictionary *outPathCookies = [[NSMutableDictionary alloc] init];
            [outDomainCookies setObject:outPathCookies forKey:path];
            for (NSString *cookieName in pathCookies) {
                NSHTTPCookie *cookie = [pathCookies objectForKey:cookieName];
                if (![cookie isSessionOnly]) {
                    [outPathCookies setObject:cookie forKey:cookieName];
                }
            }
        }
    }
    return outCookies;
}

-(void)encodeWithCoder:(NSCoder *)aCoder
{
    // Only save non-session cookies
    if (_domainGlobalCookies) {
        [aCoder encodeObject:[self removeSessionCookies:_domainGlobalCookies] forKey:@"domainGlobalCookies"];
    }

    if (_subdomainCookies) {
        [aCoder encodeObject:[self removeSessionCookies:_subdomainCookies] forKey:@"subdomainCookies"];
    }
}


#pragma mark NSCopying

-(id)copyWithZone:(NSZone *)zone
{
    BSHTTPCookieStorage* copy = [[[self class] allocWithZone:zone] init];
    if (copy) {
        copy->_subdomainCookies = [self.subdomainCookies mutableCopy];
        copy->_domainGlobalCookies = [self.domainGlobalCookies mutableCopy];
    }
    return copy;
}

@end

// ---

@implementation NSHTTPCookie (BSHTTPCookieStorage)

-(id)initWithCoder:(NSCoder *)aDecoder
{
    NSDictionary* cookieProperties = [aDecoder decodeObjectForKey:@"cookieProperties"];
    if (![cookieProperties isKindOfClass:[NSDictionary class]]) {
        // cookies are always immutable, so there's no point to return anything here if its properties cannot be found.
        return nil;
    }
    self = [self initWithProperties:cookieProperties];
    return self;
}


-(void) encodeWithCoder:(NSCoder *)aCoder
{
    NSDictionary* cookieProperties = self.properties;
    if (cookieProperties) {
        [aCoder encodeObject:cookieProperties forKey:@"cookieProperties"];
    }
}

@end

// ---
