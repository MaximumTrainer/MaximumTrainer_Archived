# Network API Reference

> **Scope:** Every HTTP request MaximumTrainer makes at runtime, covering
> `maximumtrainer.com` (the main back-end), the GitHub Releases API (update
> check), and the three third-party integrations (Strava, TrainingPeaks,
> SelfLoops).  
> **Source files audited:** `src/persistence/db/` (all DAO classes),
> `src/app/util.cpp` (JSON parsers), `src/ui/dialoglogin.cpp`,
> `src/ui/mainwindow.cpp`, `src/ui/workoutdialog.cpp`.

---

## Contents

1. [Base URLs](#1-base-urls)
2. [Session lifecycle](#2-session-lifecycle)
3. [maximumtrainer.com endpoints](#3-maximumtrainercom-endpoints)
   - [3.1 Login (WebView)](#31-login-webview)
   - [3.2 GET radio stations](#32-get-radio-stations)
   - [3.3 GET all achievements](#33-get-all-achievements)
   - [3.4 GET user achievements](#34-get-user-achievements)
   - [3.5 PUT award achievement](#35-put-award-achievement)
   - [3.6 GET sensor list](#36-get-sensor-list)
   - [3.7 PUT account settings](#37-put-account-settings)
4. [GitHub Releases API (version check)](#4-github-releases-api-version-check)
5. [Third-party integrations](#5-third-party-integrations)
   - [5.1 Strava](#51-strava)
   - [5.2 TrainingPeaks](#52-trainingpeaks)
   - [5.3 SelfLoops](#53-selfloops)
6. [Connectivity probes](#6-connectivity-probes)
7. [Offline mode](#7-offline-mode)
8. [Security notes](#8-security-notes)

---

## 1. Base URLs

| Environment | Value |
|-------------|-------|
| Production (active) | `https://maximumtrainer.com/index.php/` |
| Development (local) | `http://localhost/index.php/` |

Controlled by `current_env` in `src/persistence/db/environnement.h`.  
All REST calls use the production URL unless the constant is changed at
compile time.

---

## 2. Session lifecycle

```
App launch
  │
  ├─► version check (GitHub API)  ──► show update dialog if newer
  │
  ├─► login WebView loads
  │       https://maximumtrainer.com/login/insideMT
  │   User submits credentials; server renders a JSON response in the page.
  │   App captures page text → parses full account object.
  │
  ├─► GET radio stations  (maximumtrainer.com)
  ├─► GET all achievements  (maximumtrainer.com)
  └─► GET user achievements  (maximumtrainer.com)

Workout start
  └─► PUT account  (session validity check)
  └─► GET sensor list  (maximumtrainer.com)

Achievement unlocked
  └─► PUT achievement  (maximumtrainer.com)

App close  (online only)
  └─► PUT account  (full settings sync)
```

---

## 3. maximumtrainer.com endpoints

### 3.1 Login (WebView)

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/login/insideMT` (EN) <br> `https://maximumtrainer.com/connexion/insideMT` (FR) |
| **Method** | Browser form POST (handled by `QWebEngineView`) |
| **Auth** | User email + password typed into the login form |
| **Triggered by** | `DialogLogin` on startup |

The login page is rendered inside an embedded `QWebEngineView`.  When the
server accepts the credentials it returns the account record as a **JSON
array** in the page body.  The app reads the plain text of the page with
`page()->toPlainText()` and passes it to `Util::parseJsonObjectAccount()`.

> **Type notation used in the tables below:** `string` = received as a JSON
> string and stored as-is; `string→int` = received as a JSON string, converted
> to an integer by the parser; `string→double`, `string→bool` follow the same
> convention.  All fields that map to `bool` are stored as `"0"` / `"1"` in
> JSON.

**Received — JSON array with one object:**

| JSON field | Type | Account field populated |
|------------|------|------------------------|
| `id` | string→int | `account->id` |
| `subscription_type_id` | string→int | `account->subscription_type_id` |
| `email` | string | `account->email` |
| `password` | string | `account->password` |
| `session_mt_id` | string | `account->session_mt_id` (used as auth token in subsequent PUT requests) |
| `session_mt_expire` | string `"yyyy-MM-dd hh:mm:ss"` | `account->session_mt_expire` |
| `first_name` | string | `account->first_name` |
| `last_name` | string | `account->last_name` |
| `display_name` | string | `account->display_name` |
| `FTP` | string→int | `account->FTP` (watts) |
| `LTHR` | string→int | `account->LTHR` (bpm) |
| `minutes_rode` | string→int | `account->minutes_rode` |
| `weight_kg` | string→double | `account->weight_kg` |
| `height_cm` | string→int | `account->height_cm` |
| `trainer_curve_id` | string→int | `account->powerCurve.id` |
| `wheel_circ` | string→int | `account->wheel_circ` (mm) |
| `bike_weight_kg` | string→double | `account->bike_weight_kg` |
| `bike_type` | string→int | `account->bike_type` |
| `nb_user_studio` | string→int | `account->nb_user_studio` |
| `enable_studio_mode` | string→bool | `account->enable_studio_mode` |
| `use_pm_for_cadence` | string→bool | `account->use_pm_for_cadence` |
| `use_pm_for_speed` | string→bool | `account->use_pm_for_speed` |
| `force_workout_window_on_top` | string→bool | `account->force_workout_window_on_top` |
| `show_included_workout` | string→bool | `account->show_included_workout` |
| `show_included_course` | string→bool | `account->show_included_course` |
| `distance_in_km` | string→bool | `account->distance_in_km` |
| `strava_access_token` | string | `account->strava_access_token` |
| `strava_private_upload` | string→bool | `account->strava_private_upload` |
| `training_peaks_access_token` | string | `account->training_peaks_access_token` |
| `training_peaks_refresh_token` | string | `account->training_peaks_refresh_token` |
| `training_peaks_public_upload` | string→bool | `account->training_peaks_public_upload` |
| `selfloops_user` | string | `account->selfloops_user` |
| `selfloops_pw` | string | `account->selfloops_pw` |
| `control_trainer_resistance` | string→bool | `account->control_trainer_resistance` |
| `stop_pairing_on_found` | string→bool | `account->stop_pairing_on_found` |
| `nb_sec_pairing` | string→int | `account->nb_sec_pairing` |
| `last_index_selected_config_workout` | string→int | `account->last_index_selected_config_workout` |
| `last_tab_sub_config_selected` | string→int | `account->last_tab_sub_config_selected` |
| `tab_display0` … `tab_display7` | string | `account->tab_display[0..7]` |
| `start_trigger` | string→int | `account->start_trigger` |
| `value_cadence_start` | string→int | `account->value_cadence_start` |
| `value_power_start` | string→int | `account->value_power_start` |
| `value_speed_start` | string→int | `account->value_speed_start` |
| `show_hr_widget` … `show_speed_curve` | string→bool (×14) | corresponding `account->show_*` fields |
| `display_hr` … `display_cadence` | string→int (×4) | corresponding `account->display_*` fields |
| `display_video` | string→int | `account->display_video` |
| `show_timer_on_top` … `font_size_timer` | string→bool/int (×5) | timer display fields |
| `averaging_power` | string→int | `account->averaging_power` |
| `offset_power` | string→int | `account->offset_power` |
| `show_seperator_interval` … `show_speed_curve` | string→bool (×8) | plot overlay flags |
| `sound_player_vol` … `sound_alert_cadence_above_target` | string→int/bool (×10) | audio settings |

**Source:** `src/app/util.cpp` — `Util::parseJsonObjectAccount()`  
**DAO source:** `src/ui/dialoglogin.cpp` lines 177, 455

---

### 3.2 GET radio stations

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/radio_rest/radio/format/json` |
| **Method** | `GET` |
| **Auth** | None |
| **Triggered by** | `MainWindow` constructor (once per session) |
| **Source** | `src/persistence/db/radiodao.cpp` — `RadioDAO::getAllRadios()` |

**Received — JSON array:**

| JSON field | Type | Description |
|------------|------|-------------|
| `name` | string | Station display name |
| `genre` | string | Music genre |
| `gotAds` | string→bool | Whether the stream contains ads |
| `bitrate` | string→int | Stream bitrate (kbps) |
| `lang` | string | Language code |
| `url` | string | Stream URL |

**Source:** `src/app/util.cpp` — `Util::parseJsonRadioList()`

---

### 3.3 GET all achievements

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/achievement_rest/achievement/format/json` |
| **Method** | `GET` |
| **Auth** | None |
| **Triggered by** | `ManagerAchievement` constructor (once per session) |
| **Source** | `src/persistence/db/achievementdao.cpp` — `AchievementDAO::getLstAchievement()` |

**Received — JSON array:**

| JSON field | Type | Description |
|------------|------|-------------|
| `id` | string→int | Achievement database ID |
| `name_en` / `name_fr` | string | Achievement name (language selected at runtime) |
| `description_en` / `description_fr` | string | Description text |
| `icon_url` | string | URL of the achievement icon image |

**Source:** `src/app/util.cpp` — `Util::parseJsonAchievementList()`

---

### 3.4 GET user achievements

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/achievement_rest/achievementuser/id/{account_id}/format/json` |
| **Method** | `GET` |
| **Auth** | Account ID in URL path |
| **Triggered by** | Called immediately after [3.3](#33-get-all-achievements) completes |
| **Source** | `src/persistence/db/achievementdao.cpp` — `AchievementDAO::getLstAchievementForUser()` |

**Received — JSON array of earned achievement records:**

| JSON field | Type | Description |
|------------|------|-------------|
| `achievement_id` | string→int | ID of an achievement the user has already earned |

The parsed set (`QSet<int>`) is used to mark which achievements are already
completed in the UI.

**Source:** `src/app/util.cpp` — `Util::parseJsonAchievementListForUser()`

---

### 3.5 PUT award achievement

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/achievement_rest/achievement/` |
| **Method** | `PUT` |
| **Content-Type** | `application/x-www-form-urlencoded` |
| **Auth** | `account_id` in body |
| **Triggered by** | When the user earns a new achievement during or after a workout |
| **Source** | `src/persistence/db/achievementdao.cpp` — `AchievementDAO::putAchievement()` |

**Sent — form body:**

| Field | Value |
|-------|-------|
| `account_id` | Numeric user ID |
| `achievement_id` | ID of the achievement just earned |

**Received:** HTTP 200 with no meaningful body (fire-and-forget).

---

### 3.6 GET sensor list

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/sensor_rest/sensor/id/{account_id}/format/json` |
| **Method** | `GET` |
| **Auth** | Account ID in URL path |
| **Triggered by** | `WorkoutDialog` — at the start of each workout session |
| **Source** | `src/persistence/db/sensordao.cpp` — `SensorDAO::getActiveSensorList()` |

**Received — JSON array:**

| JSON field | Type | Description |
|------------|------|-------------|
| `ant_id` | string→int | Sensor ANT+/BLE connection ID |
| `device_type` | string→int | Sensor type code (maps to `Sensor::SENSOR_TYPE` enum) |
| `name` | string | User-assigned sensor name |
| `details` | string | Additional device details |

**`device_type` values:**

| Code | Sensor type |
|------|-------------|
| 0 | Heart rate monitor |
| 1 | Cadence sensor |
| 2 | Speed & cadence combined |
| 3 | Speed sensor |
| 4 | Power meter |
| 5 | FE-C smart trainer |
| 6 | Muscle oxygen (Moxy) |

**Source:** `src/app/util.cpp` — `Util::parseJsonSensorList()`

---

### 3.7 PUT account settings

| | |
|-|-|
| **URL** | `https://maximumtrainer.com/index.php/api/account_rest/account/` |
| **Method** | `PUT` |
| **Content-Type** | `application/x-www-form-urlencoded` |
| **Auth** | `session_mt_id` in body |
| **Triggered by** | (1) `MainWindow::closeEvent()` — on every app exit; (2) `WorkoutDialog` — before starting a workout to verify the session is still valid |
| **Source** | `src/persistence/db/userdao.cpp` — `UserDAO::putAccount()` |

This is the main settings-sync call.  It writes the entire in-memory account
state back to the server.  The **same 70+ fields** that are returned at login
([§3.1](#31-login-webview)) are sent back here:

**Sent — form body (70+ fields):**

*Identity & auth:*

| Field | Description |
|-------|-------------|
| `id` | User account ID |
| `session_mt_id` | Session token (authentication) |
| `last_lang` | Active UI language |
| `last_os` | Operating system string (e.g. `"linux"`) |

*Physical / fitness profile:*

| Field | Description |
|-------|-------------|
| `FTP` | Functional Threshold Power (watts) |
| `LTHR` | Lactate Threshold Heart Rate (bpm) |
| `minutes_rode` | Cumulative training minutes |
| `weight_kg` | Rider weight |
| `weight_in_kg` | Unit preference (`1` = kg, `0` = lbs) |
| `height_cm` | Rider height |
| `trainer_curve_id` | Trainer power curve ID |
| `wheel_circ` | Wheel circumference (mm) |
| `bike_weight_kg` | Bicycle weight (kg) |
| `bike_type` | Bike category code |

*Hardware & studio settings:*

| Field | Description |
|-------|-------------|
| `nb_user_studio` | Studio user count |
| `enable_studio_mode` | Studio mode on/off |
| `use_pm_for_cadence` | Use power meter for cadence readings |
| `use_pm_for_speed` | Use power meter for speed readings |
| `control_trainer_resistance` | ERG resistance control on/off |
| `stop_pairing_on_found` | Auto-stop sensor pairing |
| `nb_sec_pairing` | Pairing timeout (seconds) |

*Workout start trigger:*

| Field | Description |
|-------|-------------|
| `start_trigger` | `0`=cadence · `1`=power · `2`=speed · `3`=button |
| `value_cadence_start` | Cadence threshold to auto-start |
| `value_power_start` | Power threshold to auto-start |
| `value_speed_start` | Speed threshold to auto-start |

*Display / widget visibility (14 boolean fields):*

`show_hr_widget`, `show_power_widget`, `show_power_balance_widget`,
`show_cadence_widget`, `show_speed_widget`, `show_calories_widget`,
`show_oxygen_widget`, `use_virtual_speed`, `show_trainer_speed`,
`display_hr`, `display_power`, `display_power_balance`, `display_cadence`,
`display_video`

*Graph overlays (10 boolean fields):*

`show_seperator_interval`, `show_grid`,
`show_hr_target`, `show_power_target`, `show_cadence_target`, `show_speed_target`,
`show_hr_curve`, `show_power_curve`, `show_cadence_curve`, `show_speed_curve`

*Power meter adjustments:*

| Field | Description |
|-------|-------------|
| `averaging_power` | Rolling average window (0 = none) |
| `offset_power` | Manual power offset (±100 W) |

*Timer display (5 fields):*

`show_timer_on_top`, `show_interval_remaining`, `show_workout_remaining`,
`show_elapsed`, `font_size_timer`

*Workout window:*

| Field | Description |
|-------|-------------|
| `force_workout_window_on_top` | Pin window above all others |
| `show_included_workout` | Show built-in workout library |
| `show_included_course` | Show built-in course library |
| `distance_in_km` | Distance unit (`1`=km, `0`=miles) |

*UI state (10 fields):*

`last_index_selected_config_workout`, `last_tab_sub_config_selected`,
`tab_display0` … `tab_display7`

*Audio (10 fields):*

`sound_player_vol`, `enable_sound`, `sound_interval`,
`sound_pause_resume_workout`, `sound_achievement`, `sound_end_workout`,
`sound_alert_power_under_target`, `sound_alert_power_above_target`,
`sound_alert_cadence_under_target`, `sound_alert_cadence_above_target`

*Third-party OAuth tokens (stored server-side for cross-device sync):*

| Field | Description |
|-------|-------------|
| `strava_access_token` | Strava OAuth bearer token |
| `strava_private_upload` | Upload activities as private |
| `training_peaks_access_token` | TrainingPeaks bearer token |
| `training_peaks_refresh_token` | TrainingPeaks refresh token |
| `training_peaks_public_upload` | Upload as public |
| `selfloops_user` | SelfLoops email address |
| `selfloops_pw` | SelfLoops password ⚠️ |

**Received:** HTTP 200 with an empty or minimal body (the call is fire-and-forget;
the app blocks on app-close until the reply arrives before quitting).

---

## 4. GitHub Releases API (version check)

| | |
|-|-|
| **URL** | `https://api.github.com/repos/MaximumTrainer/MaximumTrainer_Redux/releases/latest` |
| **Method** | `GET` |
| **Headers** | `User-Agent: MaximumTrainer` · `Accept: application/vnd.github+json` · `X-GitHub-Api-Version: 2022-11-28` |
| **Auth** | None (public API) |
| **Timeout** | 10 seconds |
| **Triggered by** | `DialogLogin` initialisation (once per session) |
| **Source** | `src/persistence/db/versiondao.cpp` — `VersionDAO::getVersion()` |

**Received — JSON object (GitHub format):**

| JSON field | Used |
|------------|------|
| `tag_name` | Latest release version string (e.g. `"v0.0.26"`) |

The app compares `tag_name` with `APP_VERSION` (embedded at build time from
`git describe`). If a newer version exists, a dialog prompts the user to
download the update.

**Source:** `src/app/util.cpp` — `Util::parseJsonObjectVersion()`

---

## 5. Third-party integrations

These calls are only made when the user explicitly uploads an activity.
They are **not** made to `maximumtrainer.com`.

### 5.1 Strava

**Deauthorise:**

| | |
|-|-|
| **URL** | `https://www.strava.com/oauth/deauthorize` |
| **Method** | `POST` |
| **Body** | `access_token={token}` |
| **Triggered by** | User disconnects Strava in settings |

**Upload activity:**

| | |
|-|-|
| **URL** | `https://www.strava.com/api/v3/uploads` |
| **Method** | `POST` (multipart/form-data) |
| **Triggered by** | "Upload to Strava" button in post-workout dialog |

Sent fields:

| Field | Value |
|-------|-------|
| `access_token` | Strava bearer token |
| `name` | Activity name |
| `description` | Activity description + `" - Activity done with MaximumTrainer.com"` |
| `private` | `"1"` or `"0"` |
| `trainer` | Always `"1"` (indoor trainer) |
| `activity_type` | Always `"ride"` |
| `data_type` | `"fit"` |
| `file` | Binary `.fit` file |

**Check upload status:**

| | |
|-|-|
| **URL** | `https://www.strava.com/api/v3/uploads/{uploadID}` |
| **Method** | `GET` |
| **Header** | `Authorization: Bearer {access_token}` |
| **Triggered by** | Polled after upload to confirm processing |

---

### 5.2 TrainingPeaks

**Refresh token:**

| | |
|-|-|
| **URL** | `https://oauth.trainingpeaks.com/oauth/token/` |
| **Method** | `POST` |
| **Body** | `client_id=maximumtrainer` · `client_secret=…` · `grant_type=refresh_token` · `refresh_token={token}` |
| **Triggered by** | Before each upload attempt |

**Upload activity:**

| | |
|-|-|
| **URL** | `https://api.trainingpeaks.com/v1/file/` |
| **Method** | `POST` (JSON) |
| **Header** | `Authorization: Bearer {access_token}` |
| **Triggered by** | "Upload to TrainingPeaks" button |

JSON body fields:

| Field | Value |
|-------|-------|
| `UploadClient` | `"MaximumTrainer.com"` |
| `Filename` | File name |
| `Data` | Base64-encoded `.fit` file contents |
| `SetWorkoutPublic` | `"true"` or `"false"` |
| `Title` | Activity name |
| `Comment` | Activity description + `" - Activity done with MaximumTrainer.com"` |
| `Type` | `"Bike"` |

---

### 5.3 SelfLoops

**Upload activity:**

| | |
|-|-|
| **URL** | `https://www.selfloops.com/restapi/maximumtrainer/activities/upload.json` |
| **Method** | `POST` (multipart/form-data) |
| **Triggered by** | "Upload to SelfLoops" button |

Sent fields:

| Field | Value |
|-------|-------|
| `email` | SelfLoops account email |
| `pw` | SelfLoops account password |
| `fitfile` | Gzipped `.fit` file (`Content-Type: application/x-gzip`) |
| `note` | Activity note |

---

## 6. Connectivity probes

These two lightweight probes are fired during the login flow to diagnose
network problems:

| Purpose | URL | Method |
|---------|-----|--------|
| Internet connectivity check | `http://www.google.com/` | `GET` |
| Public IP address lookup | `http://bot.whatismyipaddress.com/` | `GET` |

Both use plain HTTP (not HTTPS). They are handled by
`ExtRequest::checkGoogleConnection()` and `ExtRequest::checkIpAddress()` in
`src/persistence/db/extrequest.cpp`.

---

## 7. Offline mode

When `account->isOffline` is `true` (user chose "Continue offline" at login):

- The `PUT account` call on app close is **skipped** entirely — no data is
  synced to the server.
- Sensor, achievement, and radio fetches are also skipped.
- Workout/course completion history is loaded from and saved to the local
  SQLite database (`<email_clean>.db`) and the legacy XML `.save` file only.

---

## 8. Security notes

| Issue | Severity | Location |
|-------|----------|----------|
| `selfloops_pw` is sent to `maximumtrainer.com` in cleartext (form-encoded over HTTPS) and returned from the login endpoint — the server stores a third-party password | ⚠️ Medium | `userdao.cpp` line 92; `util.cpp` `parseJsonObjectAccount` |
| Strava, TrainingPeaks, and SelfLoops credentials are persisted server-side for cross-device sync; a server breach would expose all linked third-party accounts | ⚠️ Medium | `userdao.cpp` lines 83–92 |
| TrainingPeaks OAuth `client_secret` is hardcoded in `environnement.h` | ⚠️ Medium | `src/persistence/db/environnement.h` — `CLIENT_SECRET_TP` |
| `session_mt_id` is transmitted in the PUT request body rather than an `Authorization` header, making it visible in access logs | ℹ️ Low | `userdao.cpp` line 53 |
| The two connectivity probes use plain HTTP (`http://`) | ℹ️ Low | `extrequest.cpp` lines 13, 26 |
| All `maximumtrainer.com` REST calls use HTTPS | ✅ Good | all DAO files |
