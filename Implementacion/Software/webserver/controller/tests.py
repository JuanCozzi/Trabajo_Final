from django.test import TestCase
from django.contrib.auth import get_user_model
from .models import *
from accounts.forms import *


class DeviceTests(TestCase):

    def setUp(self):
        def _save_form(form):
            if form.is_valid():
                return form.save()
            else:
                print('ERRORS ', form.errors.get_json_data())
                return None

        form = CreateDeviceForm({'username': 'pd1', 'password1': 'qwertyas', 'password2': 'qwertyas'})
        device_1 = _save_form(form)
        # form = CreateDeviceForm({'username': 'tyWj5hd34k', 'password1': 'qwertyas', 'password2': 'qwertyas'})
        # device_2 = _save_form(form)
        form = CreateApplicationUserForm({'username': 'pau1', 'email': 'a@a.com', 'device_id': 'pd1', 'password1': 'qwertyas', 'password2': 'qwertyas'})
        _save_form(form)

    # def test_asd(self):
    #     device = Device.objects.get(device=User.objects.get(username='pd1'))
    #     print('el primero ', device.connected)
    #     device.set()
    #     print('el primero ', device.connected)

    #     device2 = Device.objects.get(device=User.objects.get(username='pd1'))
    #     print('el segundo ', device2.connected)

    def test_clean_for_device(self):
        # valid device output
        output_data = {
            'ID': "0",
            'Output': "1",
            'TimeOn': "0",
            'Power': "1",
            'EnergyMax': "1",
            'HourOn': "22:50",
            'Level': "1",
            'Force': "0",
            'Control': "0",
            'Status': "2"
        }
        print('From dict ', DeviceOutput.from_dict(output_data))
        form = DeviceOutputForm(DeviceOutput.from_dict(output_data))
        self.assertTrue(form.is_valid())
        print('Cleaned ', form.cleaned_data)
        cleaned_for_device = DeviceOutput.clean_for_device(form.cleaned_data)
        print('Cleaned for device ', cleaned_for_device)
        expected = {
            'ID': 0,
            'Output': 1,
            'TimeOn': 0,
            'Power': 1,
            'EnergyMax': 1,
            'TimeMax': 60000,
            'HourOn': "22:50",
            'Level': 1,
            'Force': 0,
            'Control': 0,
            'Status': False
        }
        self.assertEqual(cleaned_for_device, expected)

    def test_create_output(self):

        # no device referenced
        form = DeviceOutputForm(DeviceOutput.from_dict({'Output': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['device_id'][0], 'This field is required.')
        print('Errors device id', form.errors['device_id'][0])

        # nonexistent device referenced
        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 1, 'Output': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['device_id'][0], 'Select a valid choice. That choice is not one of the available choices.')
        print('Errors device id', form.errors['device_id'][0])

        # no output number
        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 0}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['output'][0], 'This field is required.')
        print('Errors output number', form.errors['output'][0])

        # invalid hour on
        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 0, 'Output': 1, 'HourOn': "24:00"}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['hour_on'][0], 'Enter a valid time.')
        print('Errors hour on', form.errors['hour_on'][0])

        # ranges upper bound
        device_output_data = {
            'ID': 0,
            'Output': 11,
            'TimeOn': 1441,
            # 'HourOn',
            'Level': 3,
            'Force': 11,
            'Control': 5,
            # 'Status'
        }
        form = DeviceOutputForm(DeviceOutput.from_dict(device_output_data))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['output'][0], 'Ensure this value is less than or equal to %d.' % DeviceOutput.MAX_OUTPUTS)
        print('Errors ', form.errors['output'][0])
        self.assertEqual(form.errors['time_on'][0], 'Ensure this value is less than or equal to %d.' % DeviceOutput.MAX_TIME_ON)
        print('Errors ', form.errors['time_on'][0])
        self.assertEqual(form.errors['level'][0], 'Select a valid choice. 3 is not one of the available choices.')
        print('Errors ', form.errors['level'][0])
        self.assertEqual(form.errors['force'][0], 'Select a valid choice. 11 is not one of the available choices.')
        print('Errors ', form.errors['force'][0])
        self.assertEqual(form.errors['control'][0], 'Select a valid choice. 5 is not one of the available choices.')
        print('Errors ', form.errors['control'][0])

        # ranges lower bound
        device_output_data = {
            'ID': 0,
            'Output': 0,
            'TimeOn': -1,
            'Power': -0.1,
            'EnergyMax': -0.1,
            # 'HourOn',
            'Level': -1,
            'Force': -1,
            'Control': -1,
            # 'Status'
        }
        form = DeviceOutputForm(DeviceOutput.from_dict(device_output_data))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['output'][0], 'Ensure this value is greater than or equal to %d.' % DeviceOutput.MIN_OUTPUTS)
        print('Errors ', form.errors['output'][0])
        self.assertEqual(form.errors['time_on'][0], 'Ensure this value is greater than or equal to %d.' % DeviceOutput.MIN_TIME_ON)
        print('Errors ', form.errors['time_on'][0])
        self.assertEqual(form.errors['power'][0], 'Ensure this value is greater than or equal to %d.' % DeviceOutput.MIN_POWER)
        print('Errors ', form.errors['power'][0])
        self.assertEqual(form.errors['energy_max'][0], 'Ensure this value is greater than or equal to %d.' % DeviceOutput.MIN_ENERGY_MAX)
        print('Errors ', form.errors['energy_max'][0])
        self.assertEqual(form.errors['level'][0], 'Select a valid choice. -1 is not one of the available choices.')
        print('Errors ', form.errors['level'][0])
        self.assertEqual(form.errors['force'][0], 'Select a valid choice. -1 is not one of the available choices.')
        print('Errors ', form.errors['force'][0])
        self.assertEqual(form.errors['control'][0], 'Select a valid choice. -1 is not one of the available choices.')
        print('Errors ', form.errors['control'][0])

        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 0, 'Output': 1, 'EnergyMax': 1.1, 'Power': 0}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['__all__'][0], 'The power consumption must be specified.')
        print('Errors ', form.errors['__all__'][0])

        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 0, 'Output': 1, 'EnergyMax': 1.1, 'Power': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['__all__'][0], f'Time max must be less than {DeviceOutput.MAX_TIME_MAX}.')
        print('Errors ', form.errors['__all__'][0])

        # valid device output
        device_output_data = {
            'ID': 0,
            'Output': 1,
            'TimeOn': 0,
            'Power': 1,
            'EnergyMax': 1,
            'HourOn': "22:50",
            'Level': 1,
            'Force': 0,
            'Control': 0,
            # 'Status'
        }
        form = DeviceOutputForm(DeviceOutput.from_dict(device_output_data))
        self.assertTrue(form.is_valid())
        form.save()
        print('Hora ', form.cleaned_data['hour_on'])

        # already existent output
        form = DeviceOutputForm(DeviceOutput.from_dict({'ID': 0, 'Output': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['output'][0], 'Device output with this Output already exists.')
        print('Errors output number', form.errors['output'][0])

    def test_update_output(self):

        # creates a device output
        device_output_data = {
            'ID': 0,
            'Output': 1,
            'TimeOn': 0,
            'Power': 1,
            'EnergyMax': 1,
            # 'HourOn',
            'Level': 1,
            'Force': 0,
            'Control': 0,
            # 'Status'
        }
        form = DeviceOutputForm(DeviceOutput.from_dict(device_output_data))
        self.assertTrue(form.is_valid())
        form.save()

        # gets the device output created and updates it
        device_output = DeviceOutput.objects.get(device_id=0, output=1)

        # the output number and device_id MUST be correct, otherwise, another output will be modified or created
        device_output_data = {
            'ID': 0,
            'Output': 1,
            'Status': 1
        }
        print('Instance ', device_output.status)
        print(DeviceOutput.prepare_to_update(device_output, device_output_data))
        print('From dict ', DeviceOutput.from_dict(device_output_data))
        form = DeviceOutputForm(DeviceOutput.prepare_to_update(device_output, device_output_data), instance=device_output)
        self.assertTrue(form.is_valid())
        form.save()

        device_output_data = {
            'ID': 0,
            'Output': 1,
            'EnergyMax': 2,
        }
        form = DeviceOutputForm(DeviceOutput.prepare_to_update(device_output, device_output_data), instance=device_output)
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['__all__'][0], f'Time max must be less than {DeviceOutput.MAX_TIME_MAX}.')
        print('Errors ', form.errors['__all__'][0])

    def test_create_device_temperature(self):

        # no device referenced
        form = DeviceTemperatureForm(DeviceTemperature.from_dict({'SetTemp': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['device_id'][0], 'This field is required.')
        print('Errors device id', form.errors['device_id'][0])

        # nonexistent device referenced
        form = DeviceTemperatureForm(DeviceTemperature.from_dict({'ID': 1, 'SetTemp': 1}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['device_id'][0], 'Select a valid choice. That choice is not one of the available choices.')
        print('Errors device id', form.errors['device_id'][0])

        # ranges upper bound
        device_temperature_data = {
            'ID': 0,
            'SetTemp': 40.1
        }
        form = DeviceTemperatureForm(DeviceTemperature.from_dict(device_temperature_data))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['temperature'][0], 'Ensure this value is less than or equal to %d.' % DeviceTemperature.MAX_TEMPERATURE)
        print('Errors ', form.errors['temperature'][0])

        # ranges lower bound
        device_temperature_data = {
            'ID': 0,
            'SetTemp': -0.1
        }
        form = DeviceTemperatureForm(DeviceTemperature.from_dict(device_temperature_data))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['temperature'][0], 'Ensure this value is greater than or equal to %d.' % DeviceTemperature.MIN_TEMPERATURE)
        print('Errors ', form.errors['temperature'][0])

        # valid device_temperature data
        device_temperature_data = {
            'ID': 0,
            'SetTemp': 24.5
        }
        form = DeviceTemperatureForm(DeviceTemperature.from_dict(device_temperature_data))
        self.assertTrue(form.is_valid())
        form.save()

        # already existent device_temperature
        form = DeviceTemperatureForm(DeviceTemperature.from_dict({'ID': 0}))
        self.assertFalse(form.is_valid())
        self.assertEqual(form.errors['device_id'][0], 'Device temperature with this Device id already exists.')
        print('Errors device_temperature device id', form.errors['device_id'][0])

    def test_update_device_temperature(self):

        # valid device_temperature data
        device_temperature_data = {
            'ID': 0,
            'SetTemp': 24.5
        }
        form = DeviceTemperatureForm(DeviceTemperature.from_dict(device_temperature_data))
        self.assertTrue(form.is_valid())
        form.save()

        # gets the device_temperature created and updates it
        device_temperature = DeviceTemperature.objects.get(device_id=0)

        # the device_id MUST be correct, otherwise, another output will be modified or created
        device_temperature_data = {
            'ID': 0,
            'SetTemp': 28
        }
        form = DeviceTemperatureForm(DeviceTemperature.from_dict(device_temperature_data), instance=device_temperature)
        self.assertTrue(form.is_valid())

    # def test_prepare_output_to_send(self):
    #     def _create_output(device_output_data):
    #         form = DeviceOutputForm(DeviceOutput.from_dict(device_output_data))
    #         print(form.errors.as_json())
    #         self.assertTrue(form.is_valid())
    #         form.save()

    #     _create_output({
    #         'device_id': 0,
    #         'output': 2,
    #         'time_on': 0,
    #         'power': 1,
    #         'hour_on': "22:50",
    #         'level': 1,
    #         'force': 0,
    #         'control': 0,
    #         'status': True
    #     })
    #     _create_output({
    #         'device_id': 0,
    #         'output': 5,
    #         'time_on': 0,
    #         'power': 1,
    #         'level': 0
    #     })
    #     _create_output({
    #         'device_id': 0,
    #         'output': 6,
    #         'time_on': 11,
    #         'power': 10,
    #         'level': 2
    #     })
        
    #     new_device_output_data = {
    #         'device_id': 0,
    #         'output': 8
    #     }

    #     device_outputs_previous_to_modified = DeviceOutput.objects.filter(device_id=0, output_number__lt=new_device_output_data['output'])
    #     form = DeviceOutputForm(DeviceOutput.from_dict(new_device_output_data))
    #     self.assertTrue(form.is_valid())
    #     device_outputs_as_list = DeviceOutput.prepare_outputs_to_send(device_outputs_previous_to_modified, form.cleaned_data)

    #     print(device_outputs_as_list)
