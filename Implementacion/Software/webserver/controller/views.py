from utils.utils import json_response
from django.shortcuts import render
from django.http import Http404


TEST = {
    "Estado": [
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    ],
    "T_arranque": [
        "22:30",
        "-",
        "7:00",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-"
    ],
    "Ton": [
        60,
        0,
        0,
        30,
        0,
        0,
        5,
        0,
        0,
        0
    ],
    "Potencia":[
        2000,
        0,
        3000,
        0,
        0,
        752,
        0,
        0,
        0,
        0
    ],
    "MaxEnergia":[
        10,
        0,
        10,
        0,
        0,
        2,
        0,
        0,
        0,
        0
    ],
    "Tfuncionamiento": [
        10,
        0,
        20,
        70,
        0,
        2,
        430,
        0,
        220,
        0
    ],
    "Encendido":[
        1,
        1,
        2,
        2,
        3,
        3,
        4,
        3,
        4,
        0
    ],
    "Nivel":[
        1,
        1,
        1,
        1,
        1,
        1,
        2,
        0,
        0,
        0
    ],
    "Forzar":[
        2,
        0,
        5,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    ],
    "Hora": "22:00"
}


def test(request):
    return json_response(body=TEST)

from django.views.decorators.csrf import csrf_exempt
from rest_framework import status
from .models import *
from accounts.models import *
from controller import status_codes
import json
@csrf_exempt    
def manage_outputs(request, device_id):

    if request.method == 'POST':
        data = json.loads(request.body)
        print('Raw data ', data)
        form = DeviceOutputForm(DeviceOutput.from_dict(data))
        if form.is_valid():
            from channels.layers import get_channel_layer
            from asgiref.sync import async_to_sync
            
            channel_layer = get_channel_layer()
            print(channel_layer, "wololo")
            print('device_id ', form.cleaned_data.get('device_id').device_id)
            print('form data ', form.data)
            print('json form data ', json.dumps(form.data))
            print('form cleaned_data ', form.cleaned_data)
            # print('json form cleaned_data ', json.dumps(form.cleaned_data))
            async_to_sync(channel_layer.group_send)(str(form.cleaned_data.get('device_id').device_id), {
                'type': 'device_message',
                'text': {'data': form.data, 'msg': status_codes.OUTPUT_PARAMETERS_SET},
            })
            # form.save()

            return json_response(body=form.data)
        else:
            return json_response(message=form.errors.get_json_data(), status=status.HTTP_400_BAD_REQUEST)

    # device_id = request.GET.get('device_id')
    print('device_id ', device_id)
    device = User.objects.get(username=device_id)
    user_device = Device.objects.get(device=device)

    try:
        outputs = DeviceOutput.objects.filter(device_id=device)
        for output in outputs:
            print(output)
    except DeviceOutput.DoesNotExist:
        outputs = None

    return render(request, 'controller/manage_outputs.html', context={'device_id': device_id, 'device_name': user_device.name, 'outputs': outputs})


def get_config(request, device_id):
    print('device_id ', device_id)

    outputs = DeviceOutput.objects.filter(device_id=User.objects.get(username=device_id))
    for output in outputs:
        # from django.forms.models import model_to_dict
        # print('Model to dict ', model_to_dict(output))
        # from django.core.serializers.json import DjangoJSONEncoder
        # print('Model to dict json ', json.dumps(model_to_dict(output), cls=DjangoJSONEncoder))
        print(output)
        print('To dict ', output.to_dict())
    try:
        temperature = DeviceTemperature.objects.get(device_id=User.objects.get(username=device_id)).temperature
    except DeviceTemperature.DoesNotExist:
        temperature = None
    # from django.core import serializers
    # a=serializers.serialize('json', outputs)
    # print('Serializer ', a)
    # print(type(outputs))
    return json_response(body={'outputs': [output.to_dict() for output in outputs], 'temperature': temperature})

def _user_devices(user):
    user_devices = Device.objects.filter(user=user)
    user_devices_list = []
    for user_device in user_devices:
        user_devices_list.append({
            'id': user_device.device.username,
            'name': user_device.name,
            'connected': user_device.connected,
        })
        print(user_device.device.username)
    print(user_devices_list)

    return user_devices_list

def get_devices(request):
    return json_response(body={'devices': _user_devices(request.user)})

def user_dashboard(request):
    print('ACA')
    return render(request,
                  'controller/user_dashboard.html',
                  context={'user_devices': _user_devices(request.user)})

def device_dashboard(request, device_id):
    print('device_id ', device_id)

    valid_device_for_user = True
    try:
        valid_device_for_user = Device.objects.filter(user=request.user, device=User.objects.get(username=device_id)).exists()
    except User.DoesNotExist:
        valid_device_for_user = False

    if not valid_device_for_user:
        raise Http404('Invalid device ID or device not assigned to user')

    return render(request, 'controller/device_dashboard.html', context={'device_id': device_id})

def user(request):
    return render(request, 'controller/user.html')
