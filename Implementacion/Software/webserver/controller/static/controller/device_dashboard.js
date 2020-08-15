document.addEventListener('DOMContentLoaded', function() {
    thermometer_1 = new Thermometer();
    thermometer_1.render('thermometer1', 0, -10, 50, 0, 'Thermometer 1');
    thermometer_2 = new Thermometer();
    thermometer_2.render('thermometer2', 0, -10, 50, 0, 'Thermometer 2');
    
    // window.setInterval(getDeviceInformation, 5000);
    document.querySelector('#editTemperature').onclick = editTemperature;
    document.querySelector('#outputsManage').href = manage_outputs_url;
    
    socket = new WebSocket(`ws://${window.location.host}/ws/controller/user/${device_id}`);

    socket.onmessage = function(e) {
        console.log('User message', e)
        // data = JSON.parse(JSON.parse(e.data))
        data = JSON.parse(e.data)
        console.log('message data', data)
        // EN REALIDAD ESTO NO VA A ESTAR
        // if (data.msg === 201) {
        //     data.msg = 202;
        //     socket.send(JSON.stringify(data))
        // } else if (data.msg === 203) {
        //     data.msg = 204;
        //     socket.send(JSON.stringify(data))
        // } else if (data.msg === 11) {
        //     getDeviceConfig();
        //     $('#addOutputModal').modal('hide');
        //     $('#editOutputModal').modal('hide');
        // }
        if (data.msg === 212) {
            updateDeviceInformation(data.data);
        }
        else if (data.msg === 11) {
            getDeviceConfig();
        }
    }
    socket.onopen = function(e) {
        console.log('open', e)
        getDeviceInformation();
        // socket.send(JSON.stringify({'output_data':{'device_id': 1},'msg':200}))
    }
    socket.onerror = function(e) {
        console.log('error', e)
    }
    socket.onclose = function(e) {
        console.log('close', e)
    }
    
    getDeviceConfig();
});

// var socket = new WebSocket('ws://127.0.0.1:8000/ws/controller/user/0')
var socket;

function randomArray(length, min, max, is_int) {
    let randomArray = [];
    for (let i = 0; i < length; i++) {
        let random = Math.random() * (max - min) + min;
        if (is_int)
            random = Math.round(random);
        else
            random = random.toFixed(1);
        randomArray.push(random);
    }

    return randomArray;
}

// SOLO TESTING
var output_operating_time_cleared = 0;

// var device_socket = new WebSocket('ws://127.0.0.1:8000/ws/controller/device/0')
// var device_socket = new WebSocket(`ws://${window.location.host}/ws/controller/device/0`)

// device_socket.onmessage = function(e) {
//     console.log('Device message', e)
//     // data = JSON.parse(JSON.parse(e.data))
//     data = JSON.parse(e.data)
//     console.log('message data', data)
//     if (data.msg === 201) {
//         data.msg = 202;
//         device_socket.send(JSON.stringify(data))
//     } else if (data.msg === 203) {
//         data.msg = 204;
//         device_socket.send(JSON.stringify(data))
//     } else if (data.msg === 205) {
//         if ('OperatingTime' in data.data)
//             output_operating_time_cleared = data.data.Output;
//         data.msg = 206;
//         device_socket.send(JSON.stringify(data))
//     } else if (data.msg === 207) {
//         data.msg = 208;
//         device_socket.send(JSON.stringify(data))
//     } else if (data.msg === 211) {
//         data.msg = 212;
//         let operating_times = randomArray(10, 0, 2000, true);
//         operating_times[output_operating_time_cleared] = 0;
//         data.data = {
//             Temp: randomArray(2, 0, 40, false),
//             // OperatingTime: randomArray(10, 0, 65535, true),
//             OperatingTime: operating_times,
//         }
//         device_socket.send(JSON.stringify(data))
//     }
// }
// device_socket.onopen = function(e) {
//     console.log('open', e)
//     // device_socket.send(JSON.stringify({'output_data':{'kit_id': 1},'msg':200}))
// }
// device_socket.onerror = function(e) {
//     console.log('error', e)
// }
// device_socket.onclose = function(e) {
//     console.log('close', e)
// }
        




/**
 * @param {String} HTML representing a single element
 * @return {Element}
 */
function htmlToElement(html) {
    var template = document.createElement('template');
    html = html.trim(); // Never return a text node of whitespace as the result
    template.innerHTML = html;
    return template.content.firstChild;
}

var thermometer_1;
var thermometer_2;

var outputs_available = []

var status_on_color = "#2196F3";
var status_off_color = "#ccc";
var status_on_electrically_off_color = "#fb922f";

function getDeviceInformation() {
    socket.send(JSON.stringify({msg: 211}))
}

function updateDeviceInformation(device_information) {
    temperatures = device_information.Temp;
    operating_times = device_information.OperationTime;

    outputs_available.forEach(output => {
        let operating_time_label = document.querySelector(`#operatingTimeOutput${output}`);
        operating_time_label.textContent = parseOperatingTime(operating_times[output]);
    })

    thermometer_1.setCurrentValue(temperatures[0]);
    thermometer_2.setCurrentValue(temperatures[1]);

    console.log('Device information', device_information)
}

function parseOperatingTime(operating_time) {
    let units = {
        year: 24 * 60 * 365,
        week: 24 * 60 * 7,
        day: 24 * 60,
        hour: 60,
        minute: 1
    }
    
    let result = []
    
    for(let name in units) {
        let p = Math.floor(operating_time / units[name]);
        if (p === 1) result.push(`${p} ${name}`);
        if (p >= 2) result.push(`${p} ${name}s`);
        operating_time %= units[name];
    }

    return result.join(', ');
}

function outputStatusChanged(dataset) {
    let checkbox = document.querySelector(`#statusOutput${dataset.output}`);
    // let slider = document.querySelector(`#sliderOutput${dataset.output}`);

    // slider.style.background = checkbox.checked ? status_on_color: status_off_color;

    socket.send(JSON.stringify({'data': {Output: parseInt(dataset.output), Status: checkbox.checked}, 'msg': 205}))
}

function resetOperatingTime(output) {
    socket.send(JSON.stringify({'data': {Output: parseInt(output), OperatingTime: 0}, 'msg': 205}))
}

function getDeviceConfig() {
    $.ajax({
        url: get_device_config_url,
        type: 'GET',
    }).done(function(data) {
        let outputs_status_div = document.querySelector('#outputsStatus');
        // Clean the outputs status div.
        while (outputs_status_div.firstChild) {
            outputs_status_div.removeChild(outputs_status_div.firstChild);
        }
        
        data.outputs.forEach(output => {
            let status_color;
            if (output.Status === 1)
                status_color = status_on_color
            else if (output.Status === 2)
                status_color = status_on_electrically_off_color
            else
                status_color = status_off_color

            outputs_available.push(output.Output)

            let row = htmlToElement(`
                <div class="row" style="padding-left: 2em; padding-right: 2em; padding-top: 1em; padding-bottom: 3em;"></div>
            `)

            row.appendChild(htmlToElement(`
                <div class="row">
                    <h5>Output ${output.Output}</h5>
                </div>
            `))
            
            row.appendChild(htmlToElement(`
                <div class="row">
                    <div class="col-sm-8">
                        <div class="row">
                            <div class="col-sm-1">
                            </div>
                            <div class="col-sm-8">
                                <div class="row">
                                    <label>Operating time</label>
                                </div>
                                <div class="row">
                                    <span id="operatingTimeOutput${output.Output}"></span>
                                </div>
                            </div>
                            <div class="col-sm-1" style="padding-left: 0; padding-right: 0">
                                <a data-output="${output.Output}" role="button" onclick="resetOperatingTime(this.dataset.output)">
                                    <i class="material-icons">restore</i>
                                </a>
                            </div>
                            <div class="col-sm-2">
                            </div>
                        </div>
                    </div>
                    <div class="col-sm-4">
                        <label>Status</label>
                        <label class="switch">
                            <input type="checkbox" data-output="${output.Output}" id="statusOutput${output.Output}" onclick="outputStatusChanged(this.dataset)" ${output.Status !== 0 ? 'checked': ''}>
                            <span class="slider round" style="background: ${status_color};" id="sliderOutput${output.Output}"></span>
                        </label>
                    </div>
                </div>
            `))

            // row.appendChild(htmlToElement(`
            //     <div class="col-sm-6">
            //         <h5>Output ${output.Output}</h5>
            //     </div>
            // `))

            // row.appendChild(htmlToElement(`
            //     <div class="col-sm-6">
            //         <div class="row">
            //             <label>Operating time <span id="operatingTimeOutput${output.Output}"></span></label>
            //         </div>
            //         <div class="row">
            //             <label>Status</label>
            //             <label class="switch">
            //                 <input type="checkbox" data-output="${output.Output}" id="statusOutput${output.Output}" onclick="outputStatusChanged(this.dataset)" ${output.Status !== 0 ? 'checked': ''}>
            //                 <span class="slider round" style="background: ${status_color};" id="sliderOutput${output.Output}"></span>
            //             </label>
            //         </div>
            //     </div>
            // `))

            outputs_status_div.appendChild(row)
        });

        document.querySelector('#setTemperature').textContent = data.temperature !== null ? `${data.temperature} ºC` : 'not set'
        document.querySelector('#setTemperature').dataset.temperature = data.temperature !== null ? (data.temperature).toFixed(1) : null;

        thermometer_1.setDesiredValue(data.temperature)
        thermometer_2.setDesiredValue(data.temperature)

        // REPETIDO PARA ASEGURARME QUE SE HAGA
        getDeviceInformation()
    }).fail(function(xhr, status, error) {
        alert(error);
    });

    return false;
}

function editTemperaturePrepare() {
    document.querySelector('#currentTemperature').textContent = 
        document.querySelector('#setTemperature').dataset.temperature !== 'null' ?
        `${document.querySelector('#setTemperature').dataset.temperature} ºC` :
        'not set';
    document.querySelector('#newTemperature').value = '';
    // document.querySelector('#editTempModal').classList.add('in');
    // $('#editTempModal').modal('show');
}

function editTemperature() {
    let new_temperature = parseFloat(document.querySelector('#newTemperature').value).toFixed(1);
    let current_temperature = parseFloat(document.querySelector('#setTemperature').dataset.temperature).toFixed(1);

    if (isNaN(new_temperature)) {
        alert('Enter a valid temperature.');
        return;
    }

    if (new_temperature === current_temperature) {
        alert('Temperature wasn\'t changed.');
        return;
    }

    socket.send(JSON.stringify({'data': {'SetTemp': new_temperature}, 'msg': 207}));
}
    