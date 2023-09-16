
  ! send read version on legal.gaijin.net and in profile and save only after that

  * gamepad
  ? save read version somewhere (implement API for read saved version and for saveVersion)


# Description of API

## Current versions
via get request on https://legal.gaijin.net/api/v1/getversions?filter=gamerules,privacypolicy,eula,termsofservice
your get json of current versions of legal docs, listed in filter
{
    "status": "OK",
    "result": {
        "eula": 1681821657,
        "gamerules": 1681821726,
        "privacypolicy": 1681821489,
        "termsofservice": 1681820778,
    }
}
without filters it will show some 'default' versions.

## Get docs links
via https://legal.gaijin.net/api/v1/getdocs?filter=gamerules,privacypolicy,eula,termsofservice
you get links to current docs
{
    "status": "OK",
    "result": {
        "eula": "https://legal.gaijin.net/eula",
        "gamerules": "https://legal.gaijin.net/gamerules",
        "privacypolicy": "https://legal.gaijin.net/privacypolicy",
        "termsofservice": "https://legal.gaijin.net/termsofservice"
    }
}

## Get docs themsleves

via https://legal.gaijin.net/<language-code>/termsofservice?format=plain 
you will request terms of service doc in plain text format for chosen language
language codes are in ISO 639-1 standard language codes [xx, xx-yy, xx-zzzz]:
ru, en, zh-tw, hz-hans

## Get current user confirmed versions

POST https://auth.gaijinent.com/userinfo.php (see https://info.gaijin.lan/pages/viewpage.action?spaceKey=WEB&title=User+Info )
  meta = 1
  id = <userid>

result json (on success)
doc have versions that were confirmed by user with following API
{
  "status": "OK",
  "meta": {
    "doc" : {
      "privacypolicy":0,
      "gamerules":0,
      "eula":0,
      "termsofservice":0
    }
  }
}

## Confirm docs API

POST auth.gaijinent.com/approve_legal.php
with user JWT in headers
{docKey0: <version>, ..., dockKeyN: <version>}


# Description of what we should achieve

## New user

New user (doesnt matter - registered previously or not) HAVE to confirm legal docs before procceed to gameplay.
That means that even without connect to legal.gaijin.net user should be able to read docs and accept them.
We should store versions of these docs and save them online and locally.
If we were not able to save versions online we should save them somewhen later (for example on next login)/
If user don't want to accept documents on consoles he need to switch to 'start screen' (where the only option is to proceed again to legal window)
On PC user should see 'login screen' if he dont want to accept docs
On mobile user should see login or start screen, depending on login options.

## Old user but new documents

When docs are changed we should show 'notification of legal docs changed' window.
This window can be closed and should be shown only if docs and versions were loaded and we know what version user have confirmed.

## Manual documents open

It is not required, but good to have option to read docs again. In this case user should just request docs only on opening the window and close it also.


# Description of how it should work

After (or even before, on consoles) user has been successfully login
We request current legal docs versions and request docs by list of docs
We also request current confirmed versions of user docs.
We should have in local (better cloud saved) cache last confirmed versions AND last confirmed versions online (to check versions later).
After we receive versions AND docs (or on timeout or any other error for new user) we show legal window.
On accept button we save locally (and in cloud storage) settings. On receiving 'success' from saving them online - we save last 'online' versions in local cache also.

# Usage of sample

Sample works pretty close for how it should look for user.
Do not treat any of this as 'universal' code - this is just template.
legal-lib.nut is not supposed to be modified much (if at all)
login.nut should not be required - your game probably has login and loginData
legal-lib-ui.nut and legal-wt.ui.nut is code that is supposed to be replaced or modified heavily. First one has functions that build ui components of data
Latter is the code that glue all of this.

Probably you will keep it similar. However, pay attention to 'new user' and 'local cache' concepts.
Local cache is NOT likely be implemented like in this sample, cause you probably don't want to have another json somewhere and want to use your local settings.

! Note !

  It is not required to have 'back' or 'cancel' button in 'confirmation' window. We have it to make user way to change login,
  however it is not required (alt+f4 and other ways to close application are familiar for user)

# TODO

  1. For WT\CR\Enlisted all users that have eula version saved in their profile should get 'notification' window (not 'confirmation')
  2. It is better to do some basic navigation for gamepad, like RB\LB to switch documents
