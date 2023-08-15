
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
    
    
    const saveButton = $("#save");
    saveButton.on('click', (e) => {
        
        var allValid = true;
        allValid &= validate($('#ssid'), patterns.ssid);
        allValid &= validate($('#wifi_password'), patterns.wifi_password);
        allValid &= validate($('#ns_hostname'), patterns.ns_hostname);
        allValid &= validate($('#ns_port'), patterns.ns_port);
        allValid &= validate($('#api_secret'), patterns.api_secret);
        allValid &= validateBG();
        
        if (!allValid) {
            return;
        }
        
        const jsonString = createJson();
        uploadForm(jsonString);
    });
    
    function createJson() {
        var json = {};
        json['ssid'] = $('#ssid').val();
        json['password'] = $('#wifi_password').val();
        json['dhcp'] = true;
        json['ip'] = '';
        json['netmask'] = '';
        json['gateway'] = '';
        json['dns1'] = '';
        json['dns2'] = '';
        json['api_secret'] = $('#api_secret').val();
        json['nightscout_host'] = $('#ns_hostname').val();
        json['nightscout_port'] = parseInt($('#ns_port').val()) || 443;
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
        fetch("http://192.168.86.24/api/save", {
            method: "POST",
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
            },            
            body: json,
        })
            .then(function(res) {
                if (res?.ok) {
                    res.json().then(data => {
                        if (data.status == "ok") {
                            $("#success-alert").removeClass("d-none");
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
    fetch("config.json")
        .then(response => response.json())
        .then(data => {
            console.log(data);
            loadFormData(data);
            sleep(300).then(x => {
                $('#main_block').removeClass("collapse");
                $('#loading_block').addClass("collapse");
            });
        });    

    function loadFormData(json) {
        $('#ssid').val(json['ssid']);
        $('#wifi_password').val(json['password']);
        $('#api_secret').val(json['api_secret']);
        $('#ns_hostname').val(json['nightscout_host']);
        $('#ns_port').val((json['nightscout_port'] == 0) ? "" : (json['nightscout_port'] || "") + "");
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
