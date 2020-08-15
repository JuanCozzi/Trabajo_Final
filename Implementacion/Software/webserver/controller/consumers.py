import asyncio
import json
import datetime

from channels.consumer import AsyncConsumer
from channels.generic.websocket import AsyncJsonWebsocketConsumer
from asgiref.sync import sync_to_async
from channels.db import database_sync_to_async

from .models import *
from controller import status_codes
from accounts.models import User, Device


# class DeviceConsumer(AsyncConsumer):

#     async def websocket_connect(self, event):
#         print('connected ', event)

#         print("AAAAAAAAAAAAAAAAAAAAAA", self.channel_name)

#         device_id = self.scope['url_route']['kwargs']['device_id']

#         await self.channel_layer.group_add('%s' % device_id, self.channel_name)

#         await self.send({
#             'type': 'websocket.accept'
#         })

#         await self.send({
#             'type': 'websocket.send',
#             'text': json.dumps('Mandando mensaje al device %s' % device_id)
#         })

#     async def websocket_receive(self, event):
#         print('received ', event)

#         device_id = self.scope['url_route']['kwargs']['device_id']
#         print('device_id ', device_id)

#         data = json.loads(event['text'])
#         print('mensaje ', data['msg'])
#         if data['msg'] == 200:
#             update_result = await self.update_output(data['data'])
#             # await self.channel_layer.group_send(device_id, {
#             #     'type': 'device_message',
#             #     'text': json.dumps({'msg': 0 if update_result else 1}),
#             # })
#             response = json.dumps({'msg': 0 if update_result else 1})
#             print('response ', response)
#             await self.send({
#                 'type': 'websocket.send',
#                 'text': response
#             })

#     @database_sync_to_async
#     def update_output(self, output_data):
        
#         form = DeviceOutputForm(output_data)
#         if form.is_valid():
#             form.save()
#             return True
#         else:
#             return False

#     async def device_message(self, event):
#         print('device_message ', event)
#         print(event['text'])

#         await self.send({
#             'type': 'websocket.send',
#             'text': json.dumps(event['text'])
#         })

#     async def websocket_disconnect(self, event):
#         print('disconnected ', event)

#         device_id = self.scope['url_route']['kwargs']['device_id']
#         print('device_id ', device_id)

#         await self.channel_layer.group_discard('%s' % device_id, self.channel_name)

class DeviceConsumer(AsyncJsonWebsocketConsumer):

    async def connect(self):
        print(' Device connected ')

        print("AAAAAAAAAAAAAAAAAAAAAA", self.channel_name)

        # device_id = self.scope['url_route']['kwargs']['device_id']

        print('Headers ', self.scope['headers'])

        device = self.scope['user']
        print('Usuario ', device)

        if device.is_authenticated and device.is_device:

            device_id = device.username

            await self.channel_layer.group_add('%s' % device_id, self.channel_name)

            await self.accept()

            await self.send_json('Conectado al canal del device %s' % device_id)

            await database_sync_to_async(Device.set_connected)(device, True)
        else:
            await self.close()

    async def receive_json(self, content, **kwargs):
        print('Device received ', content)
        print('Device received kwargs ', kwargs)

        device = self.scope['user']
        print('Usuario ', device)

        if not device.is_authenticated and not device.is_device:
            return

        device_id = device.username

        # device_id = self.scope['url_route']['kwargs']['device_id']
        print('device_id ', device_id)

        # data = json.loads(event['text'])
        print('mensaje ', content)
        msg = content['msg']

        # from device messages
        if msg == status_codes.OUTPUT_ADDED_CONFIRMED_BY_DEVICE:
            # content['data']['ID'] = device_id
            add_result = await self.add_output(device_id, content['data'])
            response = {'msg': status_codes.OK if add_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.OUTPUT_DELETED_CONFIRMED_BY_DEVICE:
            update_result = await self.delete_output(device_id, content['data']['Output'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.OUTPUT_PARAMETERS_CHANGED_CONFIRMED_BY_DEVICE:
            print('OUTPUT_PARAMETERS_CHANGED_CONFIRMED_BY_DEVICE message')
            # content['data']['ID'] = device_id
            print(content['data'])
            update_result = await self.update_output(device_id, content['data'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
            # await self.send_json({'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED})
        elif msg == status_codes.TEMPERATURE_CONFIRMED_BY_DEVICE:
            update_result = await self.update_temperature(device_id, content['data']['SetTemp'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
            # await self.send_json({'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED})
        elif msg == status_codes.WORKING_TIME_AND_TEMPERATURE_GOT_FROM_DEVICE:
            response = content
        elif msg == status_codes.ALL_PARAMETERS_GOT_FROM_DEVICE:
            response = content
        elif msg == status_codes.TEMPERATURE_DEPENDENT_OUTPUT_ELECTRICALLY_ON:
            content['data']['Status'] = DeviceOutput.STATUS_ON
            update_result = await self.update_output(device_id, content['data'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.TEMPERATURE_DEPENDENT_OUTPUT_ELECTRICALLY_OFF:
            content['data']['Status'] = DeviceOutput.STATUS_ON_ELECTRICALLY_OFF
            print(content['data'])
            update_result = await self.update_output(device_id, content['data'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.MANUAL_ON:
            content['data']['Status'] = DeviceOutput.STATUS_ON if content['data']['Status'] else DeviceOutput.STATUS_OFF
            update_result = await self.update_output(device_id, content['data'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.TIME_ON:
            content['data']['Status'] = DeviceOutput.STATUS_ON if content['data']['Status'] else DeviceOutput.STATUS_OFF
            update_result = await self.update_output(device_id, content['data'])
            response = {'msg': status_codes.OK if update_result else status_codes.PARAMETER_SET_IN_SERVER_FAILED}
        elif msg == status_codes.OUTPUT_MODIFICATION_FAILED_LEVEL:
            pass
        elif msg == status_codes.OUTPUT_MODIFICATION_FAILED_ON_TIME_SURPASSED:
            pass
        elif msg == status_codes.OUTPUT_MODIFICATION_FAILED_MAX_ENERGY_SURPASSED:
            pass
        elif msg == status_codes.OUTPUT_MODIFICATION_FAILED_ELECTRIC_FAILURE:
            pass
        elif msg == status_codes.MSG_FORMAT_ERROR:
            pass
        elif msg == status_codes.SYNTAX_ERROR:
            return
        # else:
        #     response = content

        await self.channel_layer.group_send(device_id, {
            'type': 'broadcast_message',
            'text': response,
        })
        

    @database_sync_to_async
    def add_output(self, device_id, output_data):

        print('Add output ', output_data)
        form = DeviceOutputForm(DeviceOutput.from_dict(output_data))
        # else:
            # form = DeviceOutputForm(output_data)
        if form.is_valid():
            form.save()
            return True
        else:
            print(form.errors.get_json_data())
            return False


    @database_sync_to_async
    def update_output(self, device_id, output_data):
        
        print('Update output ', output_data)
        output_data['ID'] = device_id
        try:
            device_output = DeviceOutput.objects.get(device_id=User.objects.get(username=device_id), output=output_data['Output'])
        except DeviceOutput.DoesNotExist:
            # ACA VA UN ERROR, PORQUE ESTO ES UPDATE, TIENE QUE EXISTIR
            device_output = None
        # if device_output is not None:
        form = DeviceOutputForm(DeviceOutput.prepare_to_update(device_output, output_data), instance=device_output)
        # else:
            # form = DeviceOutputForm(output_data)
        if form.is_valid():
            form.save()
            return True
        else:
            print(form.errors.get_json_data())
            return False

            
    @database_sync_to_async
    def delete_output(self, device_id, output):

        print('Delete output ', output, 'from device ', 0)
        if DeviceOutput.objects.get(device_id=User.objects.get(username=device_id), output=output).delete()[0] != 1:
            print('Output not deleted')
            return False
            
        return True


    @database_sync_to_async
    def update_temperature(self, device_id, temperature):

        try:
            device_temperature = DeviceTemperature.objects.get(device_id=User.objects.get(username=device_id))
        except DeviceTemperature.DoesNotExist:
            # ACA VA UN ERROR, PORQUE ESTO ES UPDATE, TIENE QUE EXISTIR
            device_temperature = None

        form = DeviceTemperatureForm({'device_id': User.objects.get(username=device_id), 'temperature': temperature}, instance=device_temperature)
        if form.is_valid():
            form.save()
            return True
        else:
            return False

    @database_sync_to_async
    def output_turned_on(self, device_id, output_number):
        device_output = DeviceOutput.objects.get(device_id=User.objects.get(username=device_id), output_number=output_number)
        if device_output is not None:
            device_output.state = False
            device_output.save()
            return True
        else:
            return False

    async def broadcast_message(self, event):
        print('Device broadcast_message ', event)
        print(event['text'])

        await self.send_json(event['text'])

    async def websocket_disconnect(self, close_code):
        print('disconnected ', close_code)

        device = self.scope['user']
        print('Usuario ', device)

        device_id = device.username
        print('device_id ', device_id)

        await database_sync_to_async(Device.set_connected)(device, False)

        await self.channel_layer.group_discard('%s' % device_id, self.channel_name)

        # PARA AVISAR AL USUARIO QUE SE DESCONECTO EL DEVICE. SIRVE/HACE FALTA?
        await self.channel_layer.group_send(device_id, {
            'type': 'broadcast_message',
            'text': json.dumps(f'device {device_id} connection lost'),
        })

        await self.close()


class UserConsumer(AsyncJsonWebsocketConsumer):

    async def connect(self):
        print('User connected ')

        print("AAAAAAAAAAAAAAAAAAAAAA", self.channel_name)

        device_id = self.scope['url_route']['kwargs']['device_id']
        print('device_id ', device_id)

        user = self.scope['user']
        print('Usuario ', user)
        print('Usuario ', user.is_application_user)
        print('Usuario ', user.is_device)

        if user.is_authenticated \
           and user.is_application_user \
           and await self.device_belongs_to_user(user, device_id):

            await self.channel_layer.group_add('%s' % device_id, self.channel_name)

            await self.accept()

            await self.send_json('Conectado al canal del device %s' % device_id)
        else:
            await self.close()

    @database_sync_to_async
    def device_belongs_to_user(self, user, device_id):
        try:
            belongs = Device.objects.filter(user=user, device=User.objects.get(username=device_id)).exists()
        except User.DoesNotExist:
            belongs = False

        return belongs

    async def receive_json(self, content, **kwargs):
        print('User received ', content)
        print('User received ', kwargs)

        user = self.scope['user']
        print('Usuario ', user)

        device_id = self.scope['url_route']['kwargs']['device_id']
        print('device_id ', device_id)

        # data = json.loads(event['text'])
        print('mensaje ', content)
        print('mensaje json', json.dumps(content))
        msg = content['msg']

        # to device messages
        if msg == status_codes.OUTPUT_ADDED_BY_USER:
            form = DeviceOutputForm(await database_sync_to_async(DeviceOutput.from_dict)(content['data']))
            if await sync_to_async(form.is_valid)():
                content['data'] = DeviceOutput.clean_for_device(form.cleaned_data)
                response = content
            else:
                print('ADD errors ', await sync_to_async(form.errors.get_json_data)())
                response = {'msg': status_codes.MSG_FORMAT_ERROR}
        elif msg == status_codes.OUTPUT_DELETED_BY_USER:
            response = content
        elif msg == status_codes.OUTPUT_PARAMETERS_CHANGED_BY_USER:
            output = await self.get_output(device_id, content['data']['Output'])
            operating_time = None
            if 'OperatingTime' in content['data']:
                operating_time = content['data']['OperatingTime']
            content['data']['ID'] = device_id
            print('UPDATE output ', output)
            update_data = await sync_to_async(DeviceOutput.prepare_to_update)(output, content['data'])
            form = DeviceOutputForm(update_data, instance=output)
            if await sync_to_async(form.is_valid)():
                content['data'].update(DeviceOutput.clean_for_device(form.cleaned_data, fields=content['data'].keys()))
                response = content
            else:
                print('UPDATE errors ', await sync_to_async(form.errors.get_json_data)())
                response = {'msg': status_codes.MSG_FORMAT_ERROR}
        elif msg == status_codes.TEMPERATURE_SET_BY_USER:
            response = content
        elif msg == status_codes.WORKING_TIME_AND_TEMPERATURE_GET_FROM_DEVICE:
            response = content
        elif msg == status_codes.ALL_PARAMETERS_GET_FROM_DEVICE:
            response = content

        response['Hour'] = datetime.datetime.now().strftime('%H:%M:%S')

        await self.channel_layer.group_send(device_id, {
            'type': 'broadcast_message',
            'text': response,
        })

    @database_sync_to_async
    def get_output(self, device_id, output):
        return DeviceOutput.objects.get(device_id=User.objects.get(username=device_id), output=output)

    async def broadcast_message(self, event):
        print('User broadcast_message ', event)
        print('User broadcast_message json', json.dumps(event))
        print(event['text'])

        await self.send_json(event['text'])

    async def websocket_disconnect(self, close_code):
        print('disconnected ', close_code)

        device_id = self.scope['url_route']['kwargs']['device_id']
        print('device_id ', device_id)

        await self.channel_layer.group_discard('%s' % device_id, self.channel_name)

        # PARA AVISAR AL USUARIO QUE SE DESCONECTO EL DEVICE. SIRVE/HACE FALTA?
        await self.channel_layer.group_send(device_id, {
            'type': 'broadcast_message',
            'text': json.dumps(f'device {device_id} connection lost'),
        })
