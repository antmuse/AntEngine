#include "Net/Hostcheck.h"
#include "Net/NetAddress.h"
#include <stdlib.h>
#include <string.h>

namespace app {


/*
 * Match a hostname against a wildcard pattern.
 * E.g.
 *  "foo.host.com" matches "*.host.com".
 *
 * We use the matching rule described in RFC6125, section 6.4.3.
 * https://tools.ietf.org/html/rfc6125#section-6.4.3
 *
 * In addition: ignore trailing dots in the host names and wildcards, so that
 * the names are used normalized. This is what the browsers do.
 *
 * Do not allow wildcard matching on IP numbers. There are apparently
 * certificates being used with an IP address in the CN field, thus making no
 * apparent distinction between a name and an IP. We need to detect the use of
 * an IP address and not wildcard match on such names.
 *
 * NOTE: hostmatch() gets called with copied buffers so that it can modify the
 * contents at will.
 */

static bool hostmatch(char* hostname, char* pattern) {
    const char* pattern_label_end, * pattern_wildcard, * hostname_label_end;
    int wildcard_enabled;
    size_t prefixlen, suffixlen;
    //struct sockaddr_in si;
    //struct sockaddr_in6 si6;

    /* normalize pattern and hostname by stripping off trailing dots */
    size_t len = strlen(hostname);
    if (hostname[len - 1] == '.') {
        hostname[len - 1] = 0;
    }
    len = strlen(pattern);
    if (pattern[len - 1] == '.') {
        pattern[len - 1] = 0;
    }
    pattern_wildcard = strchr(pattern, '*');
    if (pattern_wildcard == NULL) {
        return 0 == AppStrNocaseCMP(pattern, hostname, 0xFF) ? true : false;
    }

    /* detect IP address as hostname and fail the match if so */
    net::NetAddress::IP ipd;
    if (net::NetAddress::convertStr2IP(hostname, ipd, false) > 0) {
        return false;
    }
    if (net::NetAddress::convertStr2IP(hostname, ipd, true) > 0) {
        return false;
    }
    /* We require at least 2 dots in pattern to avoid too wide wildcard match. */
    wildcard_enabled = 1;
    pattern_label_end = strchr(pattern, '.');
    if (pattern_label_end == NULL || strchr(pattern_label_end + 1, '.') == NULL ||
        pattern_wildcard > pattern_label_end ||
        0 == AppStrNocaseCMP(pattern, "xn--", 4)) {
        wildcard_enabled = 0;
    }
    if (!wildcard_enabled) {
        return 0 == AppStrNocaseCMP(pattern, hostname, 0xFF) ? true : false;
    }

    hostname_label_end = strchr(hostname, '.');
    if (hostname_label_end == NULL || 0 != AppStrNocaseCMP(pattern_label_end, hostname_label_end, 0xFF)) {
        return false;
    }

    /* The wildcard must match at least one character, so the left-most
       label of the hostname is at least as large as the left-most label
       of the pattern. */
    if (hostname_label_end - hostname < pattern_label_end - pattern) {
        return false;
    }
    prefixlen = pattern_wildcard - pattern;
    suffixlen = pattern_label_end - (pattern_wildcard + 1);
    return 0 == AppStrNocaseCMP(pattern, hostname, prefixlen) &&
        0 == AppStrNocaseCMP(pattern_wildcard + 1, hostname_label_end - suffixlen, suffixlen)
        ? true : false;
}

bool AppCheckHost(const char* match_pattern, const char* hostname) {
    if (match_pattern && match_pattern[0] && hostname && hostname[0]) {
        String matchp = match_pattern;
        String hostp = hostname;
        return hostmatch((s8*)hostp.c_str(), (s8*)matchp.c_str());
    }
    return false;
}


} //namespace app
