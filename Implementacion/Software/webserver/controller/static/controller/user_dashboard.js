document.addEventListener('DOMContentLoaded', function() {
});


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
        



// var socket = new WebSocket('ws://127.0.0.1:8000/ws/controller/user/0')
// var socket = new WebSocket(`ws://${window.location.host}/ws/controller/user/0`)

// socket.onmessage = function(e) {
//     console.log('User message', e)
//     // data = JSON.parse(JSON.parse(e.data))
//     data = JSON.parse(e.data)
//     console.log('message data', data)
//     // EN REALIDAD ESTO NO VA A ESTAR
//     // if (data.msg === 201) {
//     //     data.msg = 202;
//     //     socket.send(JSON.stringify(data))
//     // } else if (data.msg === 203) {
//     //     data.msg = 204;
//     //     socket.send(JSON.stringify(data))
//     // } else if (data.msg === 11) {
//     //     getDeviceConfig();
//     //     $('#addOutputModal').modal('hide');
//     //     $('#editOutputModal').modal('hide');
//     // }
//     if (data.msg === 212) {
//         updateDeviceInformation(data.data);
//     }
//     else if (data.msg === 11) {
//         getDeviceConfig();
//     }
// }
// socket.onopen = function(e) {
//     console.log('open', e)
//     getDeviceInformation();
//     // socket.send(JSON.stringify({'output_data':{'device_id': 1},'msg':200}))
// }
// socket.onerror = function(e) {
//     console.log('error', e)
// }
// socket.onclose = function(e) {
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

function deviceSelected(device_id) {
    window.location.href = device_dashboard_url.replace('123', device_id);
}
