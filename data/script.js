
(() => {
    'use strict'
    
    const patterns = {
        ssid: /^[^!#;+\]\/"\t][^+\]\/"\t]{0,30}[^ +\]\/"\t]$|^[^ !#;+\]\/"\t]$[ \t]+$/,
        wifi_password: /^.{8,}$/,
        ns_hostname: /(^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$)|(^([a-z0-9]+(-[a-z0-9]+)*\.)+[a-z]{2,}$)/,
        ns_port: /(^$)|(.{3,5})/,
        api_secret: /(^$)|(.{12,})/,
        bg_mgdl: /^[3-9][0-9]$|^[1-3][0-9][0-9]$/,
        bg_mmol: /^(([2-9])|([1-2][0-9]))(\.[1-9])?$/,
        
    };
    let configJson = {};
    
    addFocusoutValidation('ssid');
    addFocusoutValidation('wifi_password');
    addFocusoutValidation('ns_hostname');
    addFocusoutValidation('ns_port');
    addFocusoutValidation('api_secret');
    
    $('#bg_units').change( (e) => {
        validateBG();
    });
    $('#bg_low').on('focusout', (e) => {
        validateBG();
    });
    $('#bg_high').on('focusout', (e) => {
        validateBG();
    });
    $('#ns_protocol').change( (e) => {
        validateProtocol();
    });
    
    const saveButton = $("#save");
    saveButton.on('click', (e) => {
        
        var allValid = true;
        allValid &= validate($('#ssid'), patterns.ssid);
        allValid &= validate($('#wifi_password'), patterns.wifi_password);
        allValid &= validate($('#ns_hostname'), patterns.ns_hostname);
        allValid &= validate($('#ns_port'), patterns.ns_port);
        allValid &= validate($('#api_secret'), patterns.api_secret);
        allValid &= validateBG();
        allValid &= validateProtocol();
        
        if (!allValid) {
            return;
        }
        
        const jsonString = createJson();
        uploadForm(jsonString);
    });
    
    function createJson() {
        var json = configJson;
        json['ssid'] = $('#ssid').val();
        json['password'] = $('#wifi_password').val();
        json['api_secret'] = $('#api_secret').val();
        
        var url = new URL("http://bogus.url/");
        url.protocol = $('#ns_protocol').val()
        url.hostname = $('#ns_hostname').val();
        url.port = $('#ns_port').val();
        json['nightscout_url'] = url.toString();
        json['units'] = $('#bg_units').val();
        
        var bg_low = 0;
        var bg_high = 0;
        if ($('#bg_units').val() == 'mgdl') {
            bg_low = parseInt($('#bg_low').val()) || 0;
            bg_high = parseInt($('#bg_high').val()) || 0;
        } else {
            bg_low = Math.round((parseFloat($('#bg_low').val()) || 0) * 18);
            bg_high = Math.round((parseFloat($('#bg_high').val()) || 0) * 18);
            
        }
        json['low_mgdl'] = bg_low;
        json['high_mgdl'] = bg_high;
        
        return JSON.stringify(json);
    }
    
    function uploadForm(json) {
        let saveUrl = "/api/save";
        let resetUrl = "/api/reset";
        if (window.location.href.indexOf("127.0.0.1") > 0) {
            console.log("Uploading to local ESP..");
            saveUrl = "http://192.168.86.24/api/save";
            resetUrl = "http://192.168.86.24/api/reset";
        }
        fetch(saveUrl, {
            method: "POST",
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: json,
        })
        .then(function(res) {
            if (res?.ok) {
                res.json().then(data => {
                    if (data.status == "ok") {
                        $("#success-alert").removeClass("d-none");
                        
                        fetch(resetUrl, {
                            method: "POST",
                            headers: {
                                'Accept': 'application/json',
                                'Content-Type': 'application/json',
                            },
                            body: "{}",
                        });
                        
                        sleep(3000).then(() => {
                            $("#success-alert").addClass("d-none");
                        });
                    }
                    else {
                        showFailureAlert();
                    }
                });
            }
            else {
                console.log(`Response error: ${res?.status}`)
                showFailureAlert();
            }
        })
        .catch(error => {
            console.log(`Fetching error: ${error}`);
            showFailureAlert();
        });
    }
    
    function showFailureAlert() {
        $("#failure-alert").removeClass("d-none");
        sleep(3000).then(() => {
            $("#failure-alert").addClass("d-none");
        });
    }
    
    
    function validateBG() {
        var valid = true;
        if ($('#bg_units').val() == "mgdl") {
            setElementValidity($('#bg_units'), true);
            valid &= validate($('#bg_low'), patterns.bg_mgdl);
            valid &= validate($('#bg_high'), patterns.bg_mgdl);
        } else if ($('#bg_units').val() == "mmol") {
            setElementValidity($('#bg_units'), true);
            valid &= validate($('#bg_low'), patterns.bg_mmol);
            valid &= validate($('#bg_high'), patterns.bg_mmol);
        } else {
            setElementValidity($('#bg_units'), false);
            valid = false;
        }
        return valid;
    }
    
    function validateProtocol() {
        var valid = false;
        if ($('#ns_protocol').val() == "http" || $('#ns_protocol').val() == "https") {
            valid = true;
        }
        setElementValidity($('#ns_protocol'), valid);
        return valid;
    }
    
    function validate(field, regex) {
        return setElementValidity(field, regex.test(field.val()));
    }
    
    function addFocusoutValidation(fieldName) {
        const field = $(`#${fieldName}`);
        field.on('focusout', (e) => {
            validate($(e.target), patterns[fieldName])
        });
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
    function sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    var configJsonUrl = 'config.json';
    if (window.location.href.indexOf("127.0.0.1") > 0) {
        configJsonUrl = "http://192.168.86.24/config.json";
    }

    fetch(configJsonUrl)
        .then(response => response.json())
        .then(data => {
            console.log(data);
            configJson = data;
            loadFormData();
            sleep(300).then(x => {
                $('#main_block').removeClass("collapse");
                $('#loading_block').addClass("collapse");
            });
        });
    
    function loadFormData() {
        var json = configJson;
        $('#ssid').val(json['ssid']);
        $('#wifi_password').val(json['password']);
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
        if (bg_low > 0 && bg_high > 0) {
            if (json["units"] == "mgdl") {
                $('#bg_low').val(bg_low + "");
                $('#bg_high').val(bg_high + "");
            }
            if (json["units"] == "mmol") {
                $('#bg_low').val(((Math.round(bg_low / 1.8) / 10) + "").replace(",", "."));
                $('#bg_high').val(((Math.round(bg_high / 1.8) / 10) + "").replace(",", "."));
            }
        }
    }
    
    
})()
