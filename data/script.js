// included from index.html

(() => {
    'use strict'

    const clockHost = "http://192.168.86.24";

    const patterns = {
        ssid: /^[\x20-\x7E]{1,32}$/,
        wifi_password: /^.{8,}$/,
        dexcom_username: /^.{6,}$/,
        dexcom_password: /^.{8,20}$/,
        ns_hostname: /(^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$)|(^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$)/,
        ns_port: /(^$)|(.{3,5})/,
        api_secret: /(^$)|(.{12,})/,
        bg_mgdl: /^[3-9][0-9]$|^[1-3][0-9][0-9]$/,
        bg_mmol: /^(([2-9])|([1-2][0-9]))(\.[0-9])?$/,
        dexcom_server: /^(us|ous|jp)$/,
        ns_protocol: /^(http|https)$/,
        clock_timezone: /^.{2,}$/,
        time_format: /^(12|24)$/,
        email_format: /^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$/,
        not_empty: /^.{1,}$/,
        custom_nodatatimer: /^(?:[6-9]|[1-5][0-9]|60)?$/,

    };

    let configJson = {};

    addValidationHandlers();

    addButtonsHandlers();

    addAdditionalWifiTypeHandler();

    loadConfiguration();

    displayVersionInfo();

    function addButtonsHandlers() {
        $('#btn_high_alarm_try').on('click', tryAlarm);
        $('#btn_low_alarm_try').on('click', tryAlarm);
        $('#btn_urgent_low_alarm_try').on('click', tryAlarm);
        $('#btn_load_limits_from_ns').on('click', loadNightscoutData);
        $("#btn_save").on('click', validateAndSave);
        $('#additional_wifi_enable').on('change', toggleAdditionalWifiSettings);
        $('#custom_hostname_enable').on('change', toggleCustomHostnameSettings);
        $('#custom_nodatatimer_enable').on('change', toggleCustomNoDataSettings);
        $('#open_wifi_network').on('change', toggleWifiPasswordField);

    }

    function addAdditionalWifiTypeHandler() {
        $('#additional_wifi_type').on('change', function () {
            var wifiType = $(this).val();
            switch (wifiType) {
                case "wpa_psk":
                    $('#additional_ssid_cell').removeClass('d-none');
                    $('#additional_wifi_password_cell').removeClass('d-none');
                    $('#additional_wifi_username_cell').addClass('d-none');
                    break;
                case "wpa_eap":
                    $('#additional_ssid_cell').removeClass('d-none');
                    $('#additional_wifi_password_cell').removeClass('d-none');
                    $('#additional_wifi_username_cell').removeClass('d-none');
                    break;
                default:
                    $('#additional_ssid_cell').addClass('d-none');
                    $('#additional_wifi_password_cell').addClass('d-none');
                    $('#additional_wifi_username_cell').addClass('d-none');
                    break;

            };
        });
    }

    function toggleAdditionalWifiSettings() {
        const isChecked = $('#additional_wifi_enable').is(':checked');
        $('#additional_wifi_settings').toggleClass('d-none', !isChecked);
        $('#additional_wifi_type').trigger('change');
    }

    function toggleCustomHostnameSettings() {
        const isChecked = $('#custom_hostname_enable').is(':checked');
        $('#custom_hostname_settings').toggleClass('d-none', !isChecked);
    }
    
    function toggleCustomNoDataSettings() {
        const isChecked = $('#custom_nodatatimer_enable').is(':checked');
        $('#custom_nodatatimer_settings').toggleClass('d-none', !isChecked);
    }

    function toggleWifiPasswordField() {
        const isChecked = $('#open_wifi_network').is(':checked');
        const passwordField = $('#wifi_password');
        
        if (isChecked) {
            // Clear and disable the password field when open network is selected
            passwordField.val('').prop('disabled', true).removeClass('is-invalid').addClass('is-valid');
        } else {
            // Re-enable the password field when unchecked
            passwordField.prop('disabled', false);
        }
        validate(passwordField, wifiPasswordValidationPatternSelector());
    }

    function tryAlarm(e) {
        const alarmType = $(e.target).attr('id').replace('_alarm_try', '').replace('btn_', '');

        const melodyField = $(`#alarm_${alarmType}_melody`);
        const customMelody = (melodyField.val() || "").trim();

        let requestBody = { "alarmType": alarmType };
        let tryAlarmUrl = "/api/alarm";

        if (customMelody.length > 0) {
            if (!validateRtttlField(melodyField)) {
                showToastFailure("Error", "Please enter a valid RTTTL melody before testing.");
                return;
            }
            requestBody = { "rtttl": customMelody };
            tryAlarmUrl = "/api/alarm/custom";
        }

        if (window.location.href.indexOf("127.0.0.1") > 0) {
            console.log("Trying on local ESP..");
            tryAlarmUrl = clockHost + tryAlarmUrl;
        }

        fetch(tryAlarmUrl, {
            method: "POST",
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(requestBody),
        })
            .then(function (res) {
                if (res?.ok) {
                    res.json().then(data => {
                        if (data.status == "ok") {
                            showToastSuccess("Success", "You should hear the alert playing");
                        }
                        else {
                            showToastFailure("Error", "Could not play the alert");
                        }
                    });
                }
                else {
                    console.log(`Response error: ${res?.status}`)
                    showToastFailure("Error", "Could not play the alert");
                }
            })
            .catch(error => {
                console.log(`Fetching error: ${error}`);
                showToastFailure("Error", "Could not play the alert");
            });
    }

    function addValidationHandlers() {
        addFocusOutValidation('ssid');
        addFocusOutValidation('wifi_password', wifiPasswordValidationPatternSelector);

        addFocusOutValidation('clock_timezone');
        addFocusOutValidation('time_format');

        addFocusOutValidation('custom_nodatatimer');

        $('#alarm_high_enable').change((e) => { changeAlarmState(e.target) });
        $('#alarm_low_enable').change((e) => { changeAlarmState(e.target) });
        $('#alarm_urgent_low_enable').change((e) => { changeAlarmState(e.target) });

        $('#glucose_source').change(glucoseDataSourceSwitch);

        $('#bg_units').change((e) => {
            validateBG();
        });

        $('#bg_urgent_low, #bg_low, #bg_high, #bg_urgent_high').on('focusout', validateBG);

        $('#ns_protocol').change((e) => {
            validate($('#ns_protocol'), patterns.ns_protocol);
        });

    }

    function validateAndSave() {
        const allValid = validateAll();

        if (!allValid) {
            showToastFailure("Error", "Please correct the validation errors before saving");
            return;
        }

        const jsonString = createJson();
        uploadForm(jsonString);
    }

    function validateAll() {
        var allValid = true;
        allValid &= validate($('#ssid'), patterns.ssid);
        console.log("Validated ssid, result: " + allValid);
        allValid &= validate($('#wifi_password'), patterns.wifi_password);
        console.log("Validated wifi password, result: " + allValid);
        allValid &= validateGlucoseSource();
        console.log("Validated glucose source, result: " + allValid);
        allValid &= validateBG();
        console.log("Validated BG, result: " + allValid);
        allValid &= validate($('#clock_timezone'), patterns.clock_timezone);
        console.log("Validated timezone, result: " + allValid);
        allValid &= validate($('#time_format'), patterns.time_format);
        console.log("Validated time format, result: " + allValid);
        allValid &= validateAlarms();
        console.log("Validated alarms, result: " + allValid);
        allValid &= validate($('#custom_nodatatimer'), patterns.custom_nodatatimer);
        console.log("Validated custom no data timer, result: " + allValid);
        return allValid;
    }

    function glucoseDataSourceSwitch() {
        const glucoseSource = $('#glucose_source');
        const value = glucoseSource.val();
        $('#nightscout_settings_card').toggleClass("d-none", value !== "nightscout");
        $('#dexcom_settings_card').toggleClass("d-none", value !== "dexcom");
        $('#librelinkup_settings_card').toggleClass("d-none", value !== "librelinkup");

        removeFocusOutValidation('ns_hostname');
        removeFocusOutValidation('ns_port');
        removeFocusOutValidation('api_secret');
        removeFocusOutValidation('dexcom_server');
        removeFocusOutValidation('dexcom_username');
        removeFocusOutValidation('dexcom_password');
        removeFocusOutValidation('librelinkup_email');
        removeFocusOutValidation('librelinkup_password');
        removeFocusOutValidation('librelinkup_region');

        switch (value) {
            case "nightscout":
                setElementValidity(glucoseSource, true);
                addFocusOutValidation('ns_hostname');
                addFocusOutValidation('ns_port');
                addFocusOutValidation('api_secret');
                break;
            case "dexcom":
                setElementValidity(glucoseSource, true);
                addFocusOutValidation('dexcom_server');
                addFocusOutValidation('dexcom_username');
                addFocusOutValidation('dexcom_password');
                break;
            case "api":
                setElementValidity(glucoseSource, true);
                break;
            case "librelinkup":
                setElementValidity(glucoseSource, true);
                addFocusOutValidation('librelinkup_email');
                addFocusOutValidation('librelinkup_password');
                addFocusOutValidation('librelinkup_region');
                break;
            default:
                setElementValidity(glucoseSource, false);
                break;
        }
    }

    function mgdlToSelectedUnits(value) {
        if ($('#bg_units').val() == 'mmol') {
            return ((Math.round(value / 1.8) / 10) + "").replace(",", ".").replace("NaN", "");
        }
        return value.toString().replace("NaN", "") + "";
    }

    function loadNightscoutData() {
        {
            if ($('#ns_protocol').val() == "" || $('#ns_hostname').val() == "") {
                showToastFailure("Error", "Please fill in the Nightscout hostname and port before loading the thresholds");
                return;
            }
            $('#btn_load_limits_from_ns').prop('disabled', true);

            var url = new URL("http://bogus.url/");
            url.protocol = $('#ns_protocol').val()
            url.hostname = $('#ns_hostname').val();
            if ($('#ns_port').val() != "") {
                url.port = $('#ns_port').val();
            }
            url.pathname = "/api/v1/status.json";

            let fetchSettings = (url, headers) => {
                return fetch(url, {
                    method: "GET",
                    headers: headers
                })
                    .then(function (res) {
                        if (res?.ok) {
                            res.json().then(data => {
                                if (data.settings) {
                                    var settings = data.settings;
                                    $('#bg_units').val(settings.units == "mmol" ? "mmol" : "mgdl");
                                    $('#bg_low').val(mgdlToSelectedUnits(settings.thresholds.bgTargetBottom));
                                    $('#bg_high').val(mgdlToSelectedUnits(settings.thresholds.bgTargetTop));
                                    $('#bg_urgent_low').val(mgdlToSelectedUnits(settings.thresholds.bgLow));
                                    $('#bg_urgent_high').val(mgdlToSelectedUnits(settings.thresholds.bgHigh));
                                    //trigger change event to update the validation
                                    $('#bg_units').trigger('change');


                                    showToastSuccess("Success", "Blood glucose thresholds successfully loaded from Nightscout");
                                }
                                else {
                                    console.log("No settings found in Nightscout response");
                                    showToastFailure("Error", "Failed to load settings from Nightscout");
                                }
                            });
                        }
                        else {
                            console.log(`Response error: ${res?.status}`)
                            showToastFailure("Error", "Failed to load settings from Nightscout");
                        }
                        $('#btn_load_limits_from_ns').prop('disabled', false);
                    })
                    .catch(error => {
                        console.log(`Fetching error: ${error}`);
                        $('#btn_load_limits_from_ns').prop('disabled', false);
                        showToastFailure("Error", "Failed to load settings from Nightscout");
                    });
            };

            var headers = {};
            if ($('#api_secret').val() != "") {
                cr = crypto.subtle.digest("SHA-1");
                cr.update($('#api_secret').val());
                cr.digest().then(function (hash) {
                    headers["api-secret"] = hash;
                    fetchSettings(url, headers);
                });
            } else {
                fetchSettings(url, headers);
            }

            showToastSuccess("Success", "Blood glucose thresholds successfully loaded from Nightscout");
        }
    }

    function changeAlarmState(target) {
        const alarmType = $(target).attr('id').replace('_enable', '').replace('alarm_', '');
        const alarmState = $(target).is(':checked');
        $(`#alarm_${alarmType}_value`).prop('disabled', !alarmState);
        $(`#alarm_${alarmType}_snooze`).prop('disabled', !alarmState);
        $(`#alarm_${alarmType}_silence`).prop('disabled', !alarmState);
        $(`#alarm_${alarmType}_melody`).prop('disabled', !alarmState);

        if (alarmState) {
            addFocusOutValidation(`alarm_${alarmType}_value`, bgValidationPattternSelector);
            addFocusOutValidationDropDown(`alarm_${alarmType}_snooze`);
            addFocusOutValidationDropDown(`alarm_${alarmType}_silence`);
            addFocusOutValidationRtttl(`alarm_${alarmType}_melody`);
        } else {
            removeFocusOutValidation(`alarm_${alarmType}_value`);
            removeFocusOutValidationDropDown(`alarm_${alarmType}_snooze`);
            removeFocusOutValidationDropDown(`alarm_${alarmType}_silence`);
            removeFocusOutValidationRtttl(`alarm_${alarmType}_melody`);
            clearValidationStatus(`alarm_${alarmType}_value`);
            clearValidationStatus(`alarm_${alarmType}_snooze`);
            clearValidationStatus(`alarm_${alarmType}_silence`);
            clearValidationStatus(`alarm_${alarmType}_melody`);
        }
    }

    function addFocusOutValidationDropDown(fieldName) {
        const field = $(`#${fieldName}`);
        field.on('focusout', (e) => {
            validateDropDown($(e.target));
        });
    }

    function removeFocusOutValidationDropDown(fieldName) {
        const field = $(`#${fieldName}`);
        field.off('focusout');
    }

    function addFocusOutValidationRtttl(fieldName) {
        const field = $(`#${fieldName}`);
        field.on('focusout', (e) => {
            validateRtttlField($(e.target));
        });
    }

    function removeFocusOutValidationRtttl(fieldName) {
        const field = $(`#${fieldName}`);
        field.off('focusout');
    }

    function bgValidationPattternSelector() {
        if ($('#bg_units').val() == "mgdl") {
            return patterns.bg_mgdl;
        } else if ($('#bg_units').val() == "mmol") {
            return patterns.bg_mmol;
        } else {
            console.error("No BG units selected");
            return /^(invalid_pattern)$/;
        }
    }

    function wifiPasswordValidationPatternSelector() {
        if ($('#open_wifi_network').is(':checked')) {
            return /^$/;
        } else {
            return patterns.wifi_password;
        }
    }

    function validateAlarms() {
        let isValid = true;
        isValid &= validateAlarm('high');
        isValid &= validateAlarm('low');
        isValid &= validateAlarm('urgent_low');
        return isValid;
    }

    function validateAlarm(alarmType) {
        const alarmEnabled = $(`#alarm_${alarmType}_enable`).is(':checked');
        if (!alarmEnabled) {
            return true;
        }

        const valueField = $(`#alarm_${alarmType}_value`);
        let isValid = validate(valueField, bgValidationPattternSelector());
        isValid &= validateDropDown($(`#alarm_${alarmType}_snooze`));
        isValid &= validateDropDown($(`#alarm_${alarmType}_silence`));
        isValid &= validateRtttlField($(`#alarm_${alarmType}_melody`));
        return isValid;
    }

    function validateDropDown(dropDown) {
        const value = dropDown.val();
        if (value === "") {
            setElementValidity(dropDown, false);
            return false;
        } else {
            setElementValidity(dropDown, true);
            return true;
        }
    }

    function isValidRtttlString(value) {
        const trimmed = (value || "").trim();
        if (trimmed === "") {
            return true; // optional
        }

        const parts = trimmed.split(":");
        if (parts.length !== 3) {
            return false;
        }

        const name = parts[0].trim();
        const defaults = parts[1].toLowerCase();
        const melody = parts[2].trim();

        const hasDefaults = /d=\d+/.test(defaults) && /o=\d+/.test(defaults) && /b=\d+/.test(defaults);
        const nameOk = /^[a-zA-Z0-9 _-]{1,20}$/.test(name);
        const melodyOk = /^[a-grpA-GRP0-9#.,]+$/.test(melody);

        return nameOk && hasDefaults && melodyOk;
    }

    function validateRtttlField(field) {
        const valid = isValidRtttlString(field.val());
        setElementValidity(field, valid);
        return valid;
    }

    function validateGlucoseSource() {
        const glucoseSource = $('#glucose_source');
        const value = glucoseSource.val();
        let isValid = true;
        if (value === "nightscout") {
            setElementValidity(glucoseSource, true);
            isValid &= validate($('#ns_hostname'), patterns.ns_hostname);
            isValid &= validate($('#ns_port'), patterns.ns_port);
            isValid &= validate($('#api_secret'), patterns.api_secret);
            isValid &= validate($('#ns_protocol'), patterns.ns_protocol);
        } else if (value === "dexcom") {
            setElementValidity(glucoseSource, true);
            isValid &= validate($('#dexcom_server'), patterns.dexcom_server);
            isValid &= validate($('#dexcom_username'), patterns.dexcom_username);
            isValid &= validate($('#dexcom_password'), patterns.dexcom_password);
        } else if (value === "librelinkup") {
            isValid &= validate($('#librelinkup_email'), patterns.email_format);
            isValid &= validate($('#librelinkup_password'), patterns.dexcom_password);
            isValid &= validate($('#librelinkup_region'), patterns.not_empty);
        } else if (value === "api") {
            // No validation needed
        } else {
            isValid = false
            setElementValidity(glucoseSource, false);
        }
        return isValid;
    }

    function createJson() {
        var json = configJson;
        //WiFi
        json['ssid'] = $('#ssid').val();
        json['password'] = $('#wifi_password').val();

        //Glucose source
        json['data_source'] = $('#glucose_source').val();

        //Dexcom
        json['dexcom_server'] = $('#dexcom_server').val();
        json['dexcom_username'] = $('#dexcom_username').val();
        json['dexcom_password'] = $('#dexcom_password').val();

        //LibreLinkUp
        json['librelinkup_email'] = $('#librelinkup_email').val();
        json['librelinkup_password'] = $('#librelinkup_password').val();
        json['librelinkup_region'] = $('#librelinkup_region').val();

        //Nightscout
        json['api_secret'] = $('#api_secret').val();
        var url = new URL("http://bogus.url/");
        url.protocol = $('#ns_protocol').val()
        url.hostname = $('#ns_hostname').val();
        url.port = $('#ns_port').val();
        json['nightscout_url'] = url.toString();

        //Glucose settings
        json['units'] = $('#bg_units').val();
        var bg_low = 0;
        var bg_high = 0;
        var bg_urgent_low = 0;
        var bg_urgent_high = 0;

        if ($('#bg_units').val() == 'mgdl') {
            bg_low = parseInt($('#bg_low').val()) || 0;
            bg_high = parseInt($('#bg_high').val()) || 0;
            bg_urgent_low = parseInt($('#bg_urgent_low').val()) || 0;
            bg_urgent_high = parseInt($('#bg_urgent_high').val()) || 0;
        } else {
            bg_low = Math.round((parseFloat($('#bg_low').val()) || 0) * 18);
            bg_high = Math.round((parseFloat($('#bg_high').val()) || 0) * 18);
            bg_urgent_low = Math.round((parseFloat($('#bg_urgent_low').val()) || 0) * 18);
            bg_urgent_high = Math.round((parseFloat($('#bg_urgent_high').val()) || 0) * 18);

        }
        json['low_mgdl'] = bg_low;
        json['high_mgdl'] = bg_high;
        json['low_urgent_mgdl'] = bg_urgent_low;
        json['high_urgent_mgdl'] = bg_urgent_high;

        //Device settings
        var brightness = parseInt($('#brightness_level').val());
        var brightnessMode = "manual";
        if (brightness == 0) (brightness = 100); // treat 0 as 100
        if (brightness < 100) {
            brightnessMode = "manual";
        } else if (brightness == 100) {
            brightnessMode = "auto_linear";
        } else if (brightness == 101) {
            brightnessMode = "auto_dimmed";
        } else {
            brightnessMode = "unknown";
        }

        json['brightness_mode'] = brightnessMode;
        json['brightness_level'] = brightness;
        json['default_face'] = parseInt($('#default_clock_face').val());
        json['tz_libc'] = $('#clock_timezone').val();
        json['tz'] = $('#clock_timezone option:selected').text();
        json['time_format'] = $('#time_format').val();

        //Alarms

        setAlarmDataToJson(json, 'high');
        setAlarmDataToJson(json, 'low');
        setAlarmDataToJson(json, 'urgent_low');
        json['alarm_intensive_mode'] = $('#alarm_intensive_mode').is(':checked');

        // Additional WiFi
        json['additional_wifi_enable'] = $('#additional_wifi_enable').is(':checked');
        json['additional_wifi_type'] = $('#additional_wifi_type').val();
        json['additional_ssid'] = $('#additional_ssid').val();
        json['additional_wifi_username'] = $('#additional_wifi_username').val();
        json['additional_wifi_password'] = $('#additional_wifi_password').val();

        // Custom hostname
        json['custom_hostname_enable'] = $('#custom_hostname_enable').is(':checked');
        json['custom_hostname'] = $('#custom_hostname').val();

         // Custom No Data Timer
        json['custom_nodatatimer_enable'] = $('#custom_nodatatimer_enable').is(':checked');
        json['custom_nodatatimer'] = $('#custom_nodatatimer').val();

        return JSON.stringify(json);
    }

    function setAlarmDataToJson(json, alarmType) {
        const alarmEnabled = $(`#alarm_${alarmType}_enable`).is(':checked');
        json[`alarm_${alarmType}_enabled`] = alarmEnabled;

        const melody = ($(`#alarm_${alarmType}_melody`).val() || "").trim();
        json[`alarm_${alarmType}_melody`] = melody;

        if (!alarmEnabled) {
            return;
        }

        let alarmValue = $(`#alarm_${alarmType}_value`).val();
        const snooze = $(`#alarm_${alarmType}_snooze`).val();
        const silence = $(`#alarm_${alarmType}_silence`).val();

        if ($('#bg_units').val() == 'mmol') {
            alarmValue = Math.round(parseFloat(alarmValue) * 18);
        }

        json[`alarm_${alarmType}_value`] = alarmValue;
        json[`alarm_${alarmType}_snooze_interval`] = snooze;
        json[`alarm_${alarmType}_silence_interval`] = silence;
    }

    function uploadForm(json) {
        let saveUrl = "/api/save";
        let resetUrl = "/api/reset";
        if (window.location.href.indexOf("127.0.0.1") > 0) {
            console.log("Uploading to local ESP..");
            saveUrl = clockHost + saveUrl;
            resetUrl = clockHost + resetUrl;
        }
        fetch(saveUrl, {
            method: "POST",
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: json,
        })
            .then(function (res) {
                if (res?.ok) {
                    res.json().then(data => {
                        if (data.status == "ok") {
                            showToastSuccess("Saved!", "The configuration was saved. Nightscout clock will restart to apply the changes.");

                            fetch(resetUrl, {
                                method: "POST",
                                headers: {
                                    'Accept': 'application/json',
                                    'Content-Type': 'application/json',
                                },
                                body: "{}",
                            });

                        }
                        else {
                            showToastFailure("Error", "Failed to save settings");
                        }
                    });
                }
                else {
                    console.log(`Response error: ${res?.status}`)
                    showToastFailure("Error", "Failed to save settings");
                }
            })
            .catch(error => {
                console.log(`Fetching error: ${error}`);
                showToastFailure("Error", "Failed to save settings");
            });
    }

    function validateBG() {
        let valid = true;
        valid &= validate($('#bg_low'), bgValidationPattternSelector());
        valid &= validate($('#bg_high'), bgValidationPattternSelector());
        valid &= validate($('#bg_urgent_low'), bgValidationPattternSelector());
        valid &= validate($('#bg_urgent_high'), bgValidationPattternSelector());

        if (valid) {
            $('#bg_normal').val(`${$('#bg_low').val()}...${$('#bg_high').val()}`);
        }

        const bgUnits = $('#bg_units').val();
        if (bgUnits === "mgdl" || bgUnits === "mmol") {
            setElementValidity($('#bg_units'), true);
        } else {
            setElementValidity($('#bg_units'), false);
            valid = false;
        }
        return valid;
    }

    function validate(field, regex) {
        try {
        var result = setElementValidity(field, regex.test(field.val()));
        return result;
        } catch (ex) {
            console.error(ex);
        }
    }

    function removeFocusOutValidation(fieldName) {
        const field = $(`#${fieldName}`);
        field.off('focusout');
    }

    function addFocusOutValidation(fieldName, patternSelector) {
        const field = $(`#${fieldName}`);
        field.on('focusout', (e) => {
            let pattern = undefined;
            if (fieldName in patterns) {
                pattern = patterns[fieldName];
            }
            if (patternSelector !== undefined) {
                pattern = patternSelector(fieldName);
            }

            if (pattern !== undefined) {
                validate($(e.target), pattern)
            } else {
                console.error(`No pattern found for field ${fieldName}`);
            }

        });
    }

    function clearValidationStatus(fieldName) {
        const field = $(`#${fieldName}`);
        field.removeClass("is-invalid");
        field.removeClass("is-valid");
    }

    function setElementValidity(field, valid) {
        if (valid) {
            field.removeClass("is-invalid");
            field.addClass("is-valid");
        } else {
            field.removeClass("is-valid");
            field.addClass("is-invalid");
        }

        return valid;
    }

    function loadConfiguration() {

        var configJsonUrl = 'config.json?' + Date.now();
        var tzJson = "tzdata.json";

        if (window.location.href.indexOf("127.0.0.1") > 0) {
            configJsonUrl = clockHost + "/config.json?" + Date.now();
            tzJson = clockHost + "/tzdata.json?" + Date.now();

        }

        Promise.all([
            fetch(configJsonUrl),
            fetch(tzJson)
        ]).then(([configJsonData, tzJsonData]) => {
            Promise.all([configJsonData.json(), tzJsonData.json()]).then(([configJsonLocal, tzJsonLocal]) => {
                var tzSelect = $('#clock_timezone');
                for (let tzInfo of tzJsonLocal) {
                    var option = document.createElement("option");
                    option.text = tzInfo.name;
                    option.value = tzInfo.value;
                    tzSelect.append(option);
                }
                configJson = configJsonLocal;
                loadFormData();

                $('#main_block').removeClass("collapse");
                $('#loading_block').addClass("collapse");

                validateAll();

            });


        }).catch(error => {
            console.log(`Fetching error: ${error}`);
            showToastFailure("Error", "Failed to load configuration");
        });

    }

    function loadFormData() {
        var json = configJson;

        if (!json) {
            return;
        }

        //WiFi
        $('#ssid').val(json['ssid']);
        $('#wifi_password').val(json['password']);
        
        // Check open_wifi_network if password is empty
        if ((!json['password'] || json['password'].trim() === '') && json['ssid'] && json['ssid'].length > 0) {
            $('#open_wifi_network').prop('checked', true);
            $('#warning_open_network').removeClass('collapse');
            toggleWifiPasswordField();
        }

        // glucose source
        $('#glucose_source').val(json['data_source']);
        $('#glucose_source').trigger('change');

        //Dexcom
        $('#dexcom_server').val(json['dexcom_server']);
        $('#dexcom_username').val(json['dexcom_username']);
        $('#dexcom_password').val(json['dexcom_password']);

        //LibreLinkUp
        $('#librelinkup_email').val(json['librelinkup_email']);
        $('#librelinkup_password').val(json['librelinkup_password']);
        $('#librelinkup_region').val(json['librelinkup_region']);

        //Nightscout        
        $('#api_secret').val(json['api_secret']);
        var url = undefined;
        if ("canParse" in URL) {
            if (URL.canParse(json['nightscout_url'])) {
                var url = new URL(json['nightscout_url']);
            }
        } else {
            try {
                url = new URL(json['nightscout_url']);
            } catch {
                console.log("Cannoot parse saved nightscout URL");
            }
        }
        if (url) {
            $('#ns_hostname').val(url.hostname);
            $('#ns_port').val(url.port);
            $('#ns_protocol').val(url.protocol.replace(":", ""));
        }

        $('#bg_units').val(json['units']);
        var bg_low = json["low_mgdl"];
        var bg_high = json["high_mgdl"];
        var bg_urgent_low = json["low_urgent_mgdl"];
        var bg_urgent_high = json["high_urgent_mgdl"];

        //        if (bg_low > 0 && bg_high > 0 && bg_urgent_low > 0 && bg_urgent_high > 0)
        {
            $('#bg_low').val(mgdlToSelectedUnits(bg_low));
            $('#bg_high').val(mgdlToSelectedUnits(bg_high));
            $('#bg_urgent_low').val(mgdlToSelectedUnits(bg_urgent_low));
            $('#bg_urgent_high').val(mgdlToSelectedUnits(bg_urgent_high));
        }

        // Device settings
        $('#brightness_level').val(json['brightness_level']);
        $('#default_clock_face').val(json['default_face']);

        $('#time_format').val(json['time_format']);

        var tz = json['tz'];
        if (!tz) {
            tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
        }

        var tzSelect = $('#clock_timezone');
        for (var i = 0; i < tzSelect[0].length; i++) {
            if (tzSelect[0][i].text == tz) {
                tzSelect[0].selectedIndex = i;
                break;
            }
        }

        // Alarms

        loadAlarmDataFromJson(json, 'high');
        loadAlarmDataFromJson(json, 'low');
        loadAlarmDataFromJson(json, 'urgent_low');
        $('#alarm_intensive_mode').prop('checked', json['alarm_intensive_mode']);

        // Additional WiFi
        $('#additional_wifi_enable').prop('checked', json['additional_wifi_enable']);
        $('#additional_wifi_type').val(json['additional_wifi_type']);
        $('#additional_ssid').val(json['additional_ssid']);
        $('#additional_wifi_username').val(json['additional_wifi_username']);
        $('#additional_wifi_password').val(json['additional_wifi_password']);
        toggleAdditionalWifiSettings();

        // Custom hostname
        $('#custom_hostname_enable').prop('checked', json['custom_hostname_enable']);
        $('#custom_hostname').val(json['custom_hostname']);
        toggleCustomHostnameSettings();

         // Custom No Data Timer
        $('#custom_nodatatimer_enable').prop('checked', json['custom_nodatatimer_enable']);
        const nodatatimer = json['custom_nodatatimer'];
        patterns.custom_nodatatimer.test(nodatatimer) ? $('#custom_nodatatimer').val(nodatimer) 
            : $('#custom_nodatatimer').val();

        toggleCustomNoDataSettings();
        
    }

    function loadAlarmDataFromJson(json, alarmType) {
        const alarmEnabled = json[`alarm_${alarmType}_enabled`];
        $(`#alarm_${alarmType}_enable`).prop('checked', alarmEnabled);

        let alarmValue = json[`alarm_${alarmType}_value`];

        if (alarmValue % 1 === 0 && json['units'] == 'mmol') {
            alarmValue = ((Math.round(alarmValue / 1.8) / 10) + "").replace(",", ".")
        }
        $(`#alarm_${alarmType}_value`).val(alarmValue);
        $(`#alarm_${alarmType}_snooze`).val(json[`alarm_${alarmType}_snooze_interval`] || "");
        $(`#alarm_${alarmType}_silence`).val(json[`alarm_${alarmType}_silence_interval`] || "");
        $(`#alarm_${alarmType}_melody`).val(json[`alarm_${alarmType}_melody`] || "");

        changeAlarmState($(`#alarm_${alarmType}_enable`));
    }

    function showToastSuccess(title, message) {
        $('#toast_success_title').text(title);
        $('#toast_success_message').text(message);
        $('#toast_success').toast('show');
    }

    function showToastFailure(title, message) {
        $('#toast_failure_title').text(title);
        $('#toast_failure_message').text(message);
        $('#toast_failure').toast('show');
    }

    function displayVersionInfo() {

    // Version info logic

    let versionUrl = "/version.txt?";
    if (window.location.href.indexOf("127.0.0.1") !== -1) {
        versionUrl = clockHost + "/version.txt?";
    }

    fetch(versionUrl + Date.now())
        .then(res => {
            if (!res.ok) throw new Error('Failed to fetch current version');
            return res.text();
        })
        .then(currentVersion => {
            currentVersion = currentVersion.trim();
            if (!currentVersion) throw new Error('Current version is empty');
            $('#current_version').text(currentVersion);
            fetch('https://raw.githubusercontent.com/ktomy/nightscout-clock/refs/heads/main/data/version.txt?' + Date.now())
                .then(res => {
                    if (!res.ok) throw new Error('Failed to fetch latest version');
                    return res.text();
                })
                .then(latestVersion => {
                    latestVersion = latestVersion.trim();
                    if (!latestVersion) throw new Error('Latest version is empty');
                    $('#latest_version').text(latestVersion);
                    if (compareVersions(currentVersion, latestVersion) < 0) {
                        $('#update_status').html(' <a href="https://github.com/ktomy/nightscout-clock/tree/main?tab=readme-ov-file#changes" target="_blank">Changes</a>');
                        $('#update_link').removeClass('d-none');
                    } else {
                        $('#update_status').text('You are using the latest version.');
                        $('#update_link').addClass('d-none');
                    }
                })
                .catch((err) => {
                    $('#latest_version').text('Error');
                    $('#update_status').text('Could not check for updates: ' + err.message);
                    $('#update_link').addClass('d-none');
                });
        })
        .catch((err) => {
            $('#current_version').text('Error');
            $('#latest_version').text('-');
            $('#update_status').text('Could not read current version: ' + err.message);
            $('#update_link').addClass('d-none');
        });
    }
    // Simple version comparison: returns -1 if v1 < v2, 0 if equal, 1 if v1 > v2
    function compareVersions(v1, v2) {
        const a = v1.trim().split('.').map(Number);
        const b = v2.trim().split('.').map(Number);
        for (let i = 0; i < Math.max(a.length, b.length); i++) {
            const n1 = a[i] || 0;
            const n2 = b[i] || 0;
            if (n1 < n2) return -1;
            if (n1 > n2) return 1;
        }
        return 0;
    }

})()
