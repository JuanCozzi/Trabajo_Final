document.addEventListener('DOMContentLoaded', function() {
    document.querySelector('#outputAdd').onclick = addOutput;
    document.querySelector('#outputEdit').onclick = editOutput;
    document.querySelector('#singleOutputDelete').setAttribute('onclick', 'deleteOutputs(this.dataset.outputNumber)');
    // document.querySelector('#refreshOutputs').onclick = getDeviceConfig;
    // document.querySelector('#diffOutputs').onclick = diffOutputs;
    document.querySelector('#outputEditStatusSlider').onclick = function editSliderClicked() {
        sliderClicked(this);
    }

    // $('#signInModal').modal('show');

    getDeviceConfig();

    socket = new WebSocket(`ws://${window.location.host}/ws/controller/user/${device_id}`)

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
        if (data.msg === 11) {
            getDeviceConfig();
            $('#addOutputModal').modal('hide');
            $('#editOutputModal').modal('hide');
        }
    }
    socket.onopen = function(e) {
        console.log('open', e)
        // socket.send(JSON.stringify({'output_data':{'device_id': 1},'msg':200}))
    }
    socket.onerror = function(e) {
        console.log('error', e)
    }
    socket.onclose = function(e) {
        console.log('close', e)
    }
});

var socket;

var status_on_color = "#2196F3";
var status_off_color = "#ccc";
var status_on_electrically_off_color = "#fb922f";

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
//     }else if (data.msg === 205) {
//         data.msg = 206;
//         device_socket.send(JSON.stringify(data))
//     } else if (data.msg === 11) {
//         getDeviceConfig();
//         $('#addOutputModal').modal('hide');
//         $('#editOutputModal').modal('hide');
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

function selectAllOutputs() {
    pages[current_page - 1].forEach(item => {
        let checkbox = $(item).find('input')[0];
        checkbox.checked = this.checked;
    });
}

function getDeviceConfig() {
    $.ajax({
        url: get_device_config_url,
        type: 'GET',
    }).done(function(data) {
        let table = document.querySelector('#outputsTableBody');
        // Clean the table.
        while (table.firstChild) {
            table.removeChild(table.firstChild);
        }
        
        data.outputs.forEach(output => {
            let row = document.createElement('tr');
            output_cell = document.createElement('td');
            hour_on_cell = document.createElement('td');
            time_on_cell = document.createElement('td');
            power_cell = document.createElement('td');
            energy_max_cell = document.createElement('td');
            level_cell = document.createElement('td');
            force_cell = document.createElement('td');
            input_dependency_cell = document.createElement('td');
            status_cell = document.createElement('td')
    
            output_cell.appendChild(document.createTextNode(output.Output))
            hour_on_cell.appendChild(document.createTextNode(output.HourOn))
            time_on_cell.appendChild(document.createTextNode(output.TimeOn))
            power_cell.appendChild(document.createTextNode(output.Power))
            energy_max_cell.appendChild(document.createTextNode(output.EnergyMax))
            level_cell.appendChild(document.createTextNode(output.Level))
            force_cell.appendChild(document.createTextNode(output.Force))
            input_dependency_cell.appendChild(document.createTextNode(output.Control))
            status_dot = document.createElement('span')
            status_dot.classList.add('dot')

            if (output.Status === 1)
                status_dot.style.background = status_on_color
            else if (output.Status === 2)
                status_dot.style.background = status_on_electrically_off_color
            else
                status_dot.style.background = status_off_color

            status_cell.appendChild(status_dot)
    
            row.appendChild(htmlToElement(`
                <td>
                    <span class="custom-checkbox">
                        <input type="checkbox" name="options[]" value="${output.Output}">
                        <label for="checkbox1"></label>
                    </span>
                </td>
            `))
            row.appendChild(output_cell)
            row.appendChild(hour_on_cell)
            row.appendChild(time_on_cell)
            row.appendChild(power_cell)
            row.appendChild(energy_max_cell)
            row.appendChild(level_cell)
            row.appendChild(force_cell)
            row.appendChild(input_dependency_cell)
            row.appendChild(status_cell)
            row.appendChild(htmlToElement(`
                <td>
                    <a href="#editOutputModal" class="edit" data-output-number="${output.Output}" data-hour-on="${output.HourOn}" data-time-on="${output.TimeOn}"
                    data-power="${output.Power}" data-energy-max="${output.EnergyMax}" data-level="${output.Level}" data-force="${output.Force}"
                        data-input-dependency="${output.Control}" data-status="${output.Status}" onClick="editOutputPrepare(this.dataset)"
                        data-toggle="modal">
                        <i class="material-icons" data-toggle="tooltip" title="Edit">&#xE254;</i>
                    </a>
                    <a href="#deleteOutputModal" class="delete" data-output-number="${output.Output}" onclick="deleteOutputPrepare(this.dataset)" data-toggle="modal">
                        <i class="material-icons" data-toggle="tooltip" title="Delete">&#xE872;</i>
                    </a>
                </td>
            `))

            table.appendChild(row)
        });

        total_outputs_num = table.rows.length;

        // paginate();
    }).fail(function(xhr, status, error) {
        alert(error);
    });

    return false;
}

function addOutputPrepare() {
    // document.querySelector('#outputEditNumber').placeholder = ;
    document.querySelector('#outputAddHourOn').placeholder = '-';
    document.querySelector('#outputAddTimeOn').placeholder = 0;
    document.querySelector('#outputAddPower').placeholder = 0;
    document.querySelector('#outputAddEnergyMax').placeholder = 0;
    document.querySelector('#outputAddLevel').placeholder = 0;
    document.querySelector('#outputAddForce').placeholder = 0;
    document.querySelector('#outputAddInputDependency').placeholder = 0;
    document.querySelector('#outputAddStatus').checked = false;
}

function addOutput() {
    let output = parseInt(document.querySelector('#outputAddNumber').value);
    let hour_on = document.querySelector('#outputAddHourOn').value;
    let time_on = parseInt(document.querySelector('#outputAddTimeOn').value);
    let power = parseInt(document.querySelector('#outputAddPower').value);
    let energy_max = parseInt(document.querySelector('#outputAddEnergyMax').value);
    let level = parseInt(document.querySelector('#outputAddLevel').value);
    let force = parseInt(document.querySelector('#outputAddForce').value);
    let input_dependency = parseInt(document.querySelector('#outputAddInputDependency').value);
    let status = document.querySelector('#outputAddStatus').checked ? 1: 0;
    
    if(isNaN(output)){
        alert('Output number is required');
        return;
    }
    
    let output_data = {
        'ID': device_id,
        'Output': output,
        'HourOn': hour_on !== '' ? hour_on: 'xx:xx',
        'TimeOn': !isNaN(time_on) ? time_on: parseInt(document.querySelector('#outputAddTimeOn').placeholder),
        'Power': !isNaN(power) ? power: parseInt(document.querySelector('#outputAddPower').placeholder),
        'EnergyMax': !isNaN(energy_max) ? energy_max: parseInt(document.querySelector('#outputAddEnergyMax').placeholder),
        'Level': !isNaN(level) ? level: parseInt(document.querySelector('#outputAddLevel').placeholder),
        'Force': !isNaN(force) ? force: parseInt(document.querySelector('#outputAddForce').placeholder),
        'Control': !isNaN(input_dependency) ? input_dependency: parseInt(document.querySelector('#outputAddInputDependency').placeholder),
        'Status': status,
    }

    socket.send(JSON.stringify({'data': output_data, 'msg': 201}))

    // $.ajax({
    //     url: 'https://m0nj6ki5jf.execute-api.us-west-2.amazonaws.com/prod/items',
    //     type: 'POST',
    //     data: JSON.stringify(output),
    //     headers: {
    //         'Authorization': token
    //     }
    // }).done(function(data) {
    //     getDeviceConfig();
    //     alert('Output successfully added.')
    // }).fail(function(xhr, status, error) {
    //     alert(error);
    // });
    return false;
}

function sliderClicked(slider) {
    slider.dataset.checked = slider.dataset.checked === 'true' ? 'false': 'true';
    if (slider.dataset.checked === 'true') {
        if (slider.dataset.uneditedStatus === '1' || slider.dataset.uneditedStatus === '0')
            slider.style.background = status_on_color;
        else if (slider.dataset.uneditedStatus === '2')
            slider.style.background = status_on_electrically_off_color;
    } else {
        slider.style.background = status_off_color;
    }
}

function editOutputPrepare(output_data) {
    document.querySelector('#outputEditNumber').innerHTML = `Output number: ${output_data.outputNumber}`;
    document.querySelector('#outputEditNumber').dataset.outputNumber = output_data.outputNumber;
    document.querySelector('#outputEditHourOn').placeholder = output_data.hourOn;
    document.querySelector('#outputEditTimeOn').placeholder = output_data.timeOn;
    document.querySelector('#outputEditPower').placeholder = output_data.power;
    document.querySelector('#outputEditEnergyMax').placeholder = output_data.energyMax;
    document.querySelector('#outputEditLevel').placeholder = output_data.level;
    document.querySelector('#outputEditForce').placeholder = output_data.force;
    document.querySelector('#outputEditInputDependency').placeholder = output_data.inputDependency;
    document.querySelector('#outputEditStatus').dataset.previousStatus = output_data.status;
    document.querySelector('#outputEditStatus').checked = output_data.status !== '0';
    document.querySelector('#outputEditStatusSlider').dataset.uneditedStatus = output_data.status;
    document.querySelector('#outputEditStatusSlider').dataset.checked = output_data.status !== '0';
    if (output_data.status === '1')
        document.querySelector('#outputEditStatusSlider').style.background = "#2196F3";
    else if (output_data.status === '2')
        document.querySelector('#outputEditStatusSlider').style.background = "#fb922f";
    else
        document.querySelector('#outputEditStatusSlider').style.background = "#ccc";
}

function editOutput() {
    let output = {
        'ID': device_id,
        'Output': document.querySelector('#outputEditNumber').dataset.outputNumber
    }

    let new_hour_on = document.querySelector('#outputEditHourOn').value;
    let new_time_on = parseInt(document.querySelector('#outputEditTimeOn').value);
    let new_power = parseInt(document.querySelector('#outputEditPower').value);
    let new_energy_max = parseInt(document.querySelector('#outputEditEnergyMax').value);
    let new_level = parseInt(document.querySelector('#outputEditLevel').value);
    let new_force = parseInt(document.querySelector('#outputEditForce').value);
    let new_input_dependency = parseInt(document.querySelector('#outputEditInputDependency').value);
    let new_status;
    if (document.querySelector('#outputEditStatus').checked) {
        if (document.querySelector('#outputEditStatus').dataset.previousStatus === '0')
            new_status = 1;
        else
            new_status = parseInt(document.querySelector('#outputEditStatus').dataset.previousStatus);
    } else {
        new_status = 0;
    }
    
    let change_made = false;

    if (new_hour_on !== '' && new_hour_on !== document.querySelector('#outputEditHourOn').placeholder)
    {
        change_made = true;
        output.HourOn = new_hour_on;
    }

    if (!isNaN(new_time_on) && new_time_on !== parseInt(document.querySelector('#outputEditTimeOn').placeholder))
    {
        change_made = true;
        output.TimeOn = new_time_on;
    }

    if (!isNaN(new_power) && new_power !== parseInt(document.querySelector('#outputEditPower').placeholder))
    {
        change_made = true;
        output.Power = new_power;
    }

    if (!isNaN(new_energy_max) && new_energy_max !== parseInt(document.querySelector('#outputEditEnergyMax').placeholder))
    {
        change_made = true;
        output.EnergyMax = new_energy_max;
    }

    if (!isNaN(new_level) && new_level !== parseInt(document.querySelector('#outputEditLevel').placeholder))
    {
        change_made = true;
        output.Level = new_level;
    }

    if (!isNaN(new_force) && new_force !== parseInt(document.querySelector('#outputEditForce').placeholder))
    {
        change_made = true;
        output.Force = new_force;
    }

    if (!isNaN(new_input_dependency) && new_input_dependency !== parseInt(document.querySelector('#outputEditInputDependency').placeholder))
    {
        change_made = true;
        output.Control = new_input_dependency;
    }

    if (new_status !== parseInt(document.querySelector('#outputEditStatus').dataset.previousStatus))
    {
        change_made = true;
        output.Status = new_status;
    }
    
    if (!change_made) {
        alert('Nothing to update.');
        return;
    }    

    socket.send(JSON.stringify({'data': output, 'msg': 205}))

    // $.ajax({
    //     url: 'https://m0nj6ki5jf.execute-api.us-west-2.amazonaws.com/prod/items',
    //     type: 'PUT',
    //     data: JSON.stringify(output),
    //     headers: {
    //         'Authorization': token
    //     }
    // }).done(function(data) {
    //     getDeviceConfig();
    //     alert('Output successfully updated.')
    // }).fail(function(xhr, status, error) {
    //     alert(error);
    // });
    return false;
}

function deleteOutputPrepare(output_data) {
    document.querySelector('#outputDelete').innerHTML = `Are you sure you want to delete output ${output_data.outputNumber}?`;
    document.querySelector('#singleOutputDelete').dataset.outputNumber = output_data.outputNumber;
}

function deleteOutputs(output) {
    socket.send(JSON.stringify({
        'data': {
            'ID': device_id,
            'Output': parseInt(output)
        },
        'msg': 203}))
}
