/*
 * CSRFToken.cpp
 *
 * Copyright (C) 2009-19 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include <core/http/CSRFToken.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <core/http/Cookie.hpp>
#include <core/http/Request.hpp>
#include <core/http/Response.hpp>

#include <core/system/System.hpp>

using namespace rstudio::core;

namespace rstudio {
namespace core {
namespace http {

void setCSRFTokenCookie(const http::Request& request, 
                        const boost::optional<boost::gregorian::days>& expiry,
                        const std::string& token,
                        const std::string& path,
                        bool secure,
                        bool iFrameEmbedding,
                        bool legacyCookies,
                        bool iFrameLegacyCookies,
                        http::Response* pResponse)
{
   boost::optional<boost::posix_time::time_duration> expiresFromNow;
   if (expiry.is_initialized())
      expiresFromNow = boost::posix_time::time_duration(24 * expiry->days(), 0, 0);

   setCSRFTokenCookie(request,
                      expiresFromNow,
                      token,
                      path,
                      secure,
                      iFrameEmbedding,
                      legacyCookies,
                      iFrameLegacyCookies,
                      pResponse);
}

void setCSRFTokenCookie(const http::Request& request,
                        const boost::optional<boost::posix_time::time_duration>& expiresFromNow,
                        const std::string& token,
                        const std::string& path,
                        bool secure,
                        bool iFrameEmbedding,
                        bool legacyCookies,
                        bool iFrameLegacyCookies,
                        http::Response* pResponse)
{
   // generate UUID for token if unspecified
   std::string csrfToken(token);
   if (csrfToken.empty())
      csrfToken = core::system::generateUuid();

   // generate CSRF token cookie
   http::Cookie cookie(
            request,
            kCSRFTokenCookie,
            csrfToken,
            path,
            Cookie::selectSameSite(legacyCookies, iFrameEmbedding),
            true, // HTTP only
            secure);

   // set expiration for cookie
   if (expiresFromNow.is_initialized())
      cookie.setExpires(*expiresFromNow);

   pResponse->addCookie(cookie, iFrameLegacyCookies);
}

bool validateCSRFForm(const http::Request& request, 
                      http::Response* pResponse,
                      bool iFrameLegacyCookies)
{
   // extract token from HTTP cookie (set above)
   std::string headerToken = request.cookieValue(kCSRFTokenCookie, iFrameLegacyCookies);
   http::Fields fields;

   // parse the form and check for a matching token
   http::util::parseForm(request.body(), &fields);
   std::string bodyToken = http::util::fieldValue<std::string>(fields,
         kCSRFTokenCookie, "");

   // report an error if they don't match
   if (headerToken.empty() || bodyToken != headerToken) 
   {
      pResponse->setStatusCode(http::status::BadRequest);
      pResponse->setBody("Missing or incorrect token.");
      return false;
   }

   // all is well
   return true;
}

bool validateCSRFHeaders(const http::Request& request, bool iFrameLegacyCookies)
{
   std::string headerToken = request.headerValue(kCSRFTokenHeader);
   std::string cookieToken = request.cookieValue(kCSRFTokenCookie, iFrameLegacyCookies);
   if (headerToken.empty() || headerToken != cookieToken)
   {
      return false;
   }
   return true;
}

} // namespace http
} // namespace core
} // namespace rstudio
