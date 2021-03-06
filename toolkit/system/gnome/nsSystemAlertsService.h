/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsSystemAlertsService_h__
#define nsSystemAlertsService_h__

#include "nsIAlertsService.h"
#include "nsCOMPtr.h"

class nsSystemAlertsService : public nsIAlertsService
{
public:
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_ISUPPORTS

  nsSystemAlertsService();
  virtual ~nsSystemAlertsService();

  nsresult Init();

protected:

};

#endif /* nsSystemAlertsService_h__ */
