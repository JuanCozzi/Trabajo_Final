from channels.testing import WebsocketCommunicator
from channels.layers import get_channel_layer
from channels.db import database_sync_to_async
import pytest
import json

from webserver.routing import application
from controller import status_codes


TEST_CHANNEL_LAYERS = {
    'default': {
        'BACKEND': 'channels.layers.InMemoryChannelLayer'
    }
}


@pytest.mark.asyncio
@pytest.mark.django_db(transaction=True)
class TestKitConsumer:

    @database_sync_to_async
    def setUp(self):
        from django.contrib.auth import get_user_model
        from .models import Kit

        User = get_user_model()
        user = User.objects.create_user(email='test@test.com', password='test')
        print(user)

        kit = Kit.objects.create(kit_id=0, user=user)

    async def test_can_connect_to_server(self, settings):
        settings.CHANNELS_LAYERS = TEST_CHANNEL_LAYERS
        communicator = WebsocketCommunicator(application=application, path='/controller/ws/0')
        connected, _ = await communicator.connect()

        assert connected

        await communicator.disconnect()

    async def test_messages(self, settings):        
        async def _communicators_assert_received(expected):
            for communicator in communicators:
                received = await communicator.receive_json_from()
                assert received == expected, f'Obtained {received}\nExpected {expected}'
        
        await self.setUp()
        settings.CHANNELS_LAYERS = TEST_CHANNEL_LAYERS
        KIT_ID = 0
        communicators = []
        kit_communicator = WebsocketCommunicator(application=application, path=f'/controller/ws/{KIT_ID}')
        connected, _ = await kit_communicator.connect()
        assert connected
        communicators.append(kit_communicator)
        
        user_communicator = WebsocketCommunicator(application=application, path=f'/controller/ws/{KIT_ID}')
        connected, _ = await user_communicator.connect()
        assert connected
        communicators.append(user_communicator)


        # response = await kit_communicator.receive_json_from()
        # assert response == 'Conectado al canal del kit 0', response
        # response = await user_communicator.receive_json_from()
        # assert response == 'Conectado al canal del kit 0', response
        await _communicators_assert_received('Conectado al canal del kit 0')

        user_to_kit_message = {
            'data': {
                'kit_id': KIT_ID,
                'output_number': 1
            },
            'msj': status_codes.OUTPUT_PARAMETERS_SET_BY_USER
        }

        # channel_layer = get_channel_layer()
        # await channel_layer.group_send(str(KIT_ID), {
        #     'type': 'broadcast_message',
        #     'text': user_to_kit_message
        # })
        await user_communicator.send_json_to(user_to_kit_message)

        await _communicators_assert_received(user_to_kit_message)

        # response = await kit_communicator.receive_json_from()
        # print('RESPUESTA ', response)
        # assert response == kit_message, f'Obtained {response}\nExpected {kit_message}'
        
        kit_to_user_message = {
            'data': {
                'kit_id': KIT_ID,
                'output_number': 1
            },
            'msj': status_codes.OUTPUT_PARAMETERS_CONFIRMED_BY_KIT
        }
        await kit_communicator.send_json_to(kit_to_user_message)

        expected = {
            'msj': status_codes.OK
        }
        await _communicators_assert_received(expected)

        for communicator in communicators:
            await communicator.disconnect()
