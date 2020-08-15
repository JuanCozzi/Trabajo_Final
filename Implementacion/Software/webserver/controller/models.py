from django.db import models
from django.conf import settings
from django.core.validators import RegexValidator, MinValueValidator, MaxValueValidator
from django.utils.translation import ugettext_lazy as _

from accounts.models import User


# alphanumeric = RegexValidator(r'^[0-9a-zA-Z]*$', 'Only alphanumeric characters are allowed.')

class RangedIntegerField(models.IntegerField):

    def __init__(self, min_value=None, max_value=None, **kwargs):
        self.min_value = min_value
        self.max_value = max_value
        if 'validators' in kwargs:
            validators = kwargs['validators']
        else:
            validators = []
        if min_value:
            validators.append(MinValueValidator(min_value))
        if max_value:
            validators.append(MaxValueValidator(max_value))
        kwargs['validators'] = validators
        super(RangedIntegerField, self).__init__(**kwargs)

    def formfield(self, **kwargs):
        context = {'min_value': self.min_value, 'max_value': self.max_value}
        context.update(kwargs)
        return super(RangedIntegerField, self).formfield(**context)

class RangedFloatField(models.FloatField):

    def __init__(self, min_value=None, max_value=None, **kwargs):
        self.min_value = min_value
        self.max_value = max_value
        if 'validators' in kwargs:
            validators = kwargs['validators']
        else:
            validators = []
        if min_value:
            validators.append(MinValueValidator(min_value))
        if max_value:
            validators.append(MaxValueValidator(max_value))
        kwargs['validators'] = validators
        super(RangedFloatField, self).__init__(**kwargs)

    def formfield(self, **kwargs):
        context = {'min_value': self.min_value, 'max_value': self.max_value}
        context.update(kwargs)
        return super(RangedFloatField, self).formfield(**context)

# class Device(models.Model):
    
#     MAX_KITS_NUM = 2**16 - 1
#     MIN_KITS_NUM = 0
    
#     device_id = RangedIntegerField(primary_key=True, min_value=MIN_KITS_NUM, max_value=MAX_KITS_NUM)
#     user = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.CASCADE)


class DeviceOutput(models.Model):
    device_id = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.CASCADE)
    # device_id = models.ForeignKey(Device, on_delete=models.CASCADE, validators=[alphanumeric])

    MAX_OUTPUTS = 10
    MIN_OUTPUTS = 1
    MAX_INPUTS = 4
    MIN_INPUTS = 1
    # minutes, ATmega integer -> 2 bytes
    MAX_TIME_MAX = 2**16 - 1
    MIN_TIME_MAX = 0
    # minutes, 0 for no time dependency, 1 day max
    MAX_TIME_ON = 1440
    MIN_TIME_ON = 0

    MIN_POWER = 0
    MIN_ENERGY_MAX = 0

    LEVEL_DEPENDENCY_NONE = 0
    LEVEL_DEPENDENCY_POSITIVE = 1
    LEVEL_DEPENDENCY_NEGATIVE = 2
    LEVEL_CHOICES = [
        (LEVEL_DEPENDENCY_NONE, 'No level dependency'),
        (LEVEL_DEPENDENCY_POSITIVE, 'Positive level dependency'),
        (LEVEL_DEPENDENCY_NEGATIVE, 'Negative level dependency'),
    ]
    # 0 doesn't force an output
    FORCE_CHOICES = [(i, 'Force output Nº%d' % i if i > 0 else 'Don\'t force an output') for i in range(0, MAX_OUTPUTS + 1)]
    # if 0 the output has no input dependency
    INPUT_ON_CHOICES = [(i, 'Dependency on input Nº%d' % i if i > 0 else 'No input dependency') for i in range(0, MAX_INPUTS + 1)]

    STATUS_OFF = 0
    STATUS_ON = 1
    STATUS_ON_ELECTRICALLY_OFF = 2
    STATUS_CHOICES = [
        (STATUS_OFF, 'Output off'),
        (STATUS_ON, 'Output on'),
        (STATUS_ON_ELECTRICALLY_OFF, 'Output off by temperature'),
    ]

    # output = models.IntegerField(primary_key=True, validators=[MinValueValidator(1), MaxValueValidator(MAX_OUTPUTS)])
    output = RangedIntegerField(primary_key=True, min_value=MIN_OUTPUTS, max_value=MAX_OUTPUTS)
    name = models.CharField(_('Descriptive name'), max_length=150, null=True, blank=True)
    # tiempo que quiero que esté encendido
    time_on = RangedIntegerField(null=True, blank=True, default=0, min_value=MIN_TIME_ON, max_value=MAX_TIME_ON)
    # time_max = RangedIntegerField(null=True, blank=True, default=0, min_value=MIN_TIME_MAX, max_value=MAX_TIME_MAX)
    power = RangedFloatField(null=True, blank=True, default=0, min_value=MIN_POWER)
    energy_max = RangedFloatField(null=True, blank=True, default=0, min_value=MIN_ENERGY_MAX)
    hour_on = models.TimeField(null=True, blank=True)
    level = models.IntegerField(null=True, blank=True, default=0, choices=LEVEL_CHOICES)
    force = models.IntegerField(null=True, blank=True, default=0, choices=FORCE_CHOICES)
    control = RangedIntegerField(null=True, blank=True, default=0, choices=INPUT_ON_CHOICES)
    # whether it's on or off
    status = models.IntegerField(null=True, blank=True, default=0, choices=STATUS_CHOICES)

    @classmethod
    def prepare_to_update(cls, instance, new_data):
        instance_dict = cls.to_dict(instance)
        print('Instance Dict ', instance_dict)
        instance_dict.update(new_data)
        print('Instance Dict updated ', instance_dict)
        return cls.from_dict(instance_dict)

    @classmethod
    def from_dict(cls, device_output_dict):
        device_output_cleaned_dict = {}

        if 'ID' in device_output_dict:
            device_output_cleaned_dict['device_id'] = User.objects.get(username=device_output_dict['ID'])
        if 'Output' in device_output_dict:
            device_output_cleaned_dict['output'] = device_output_dict['Output']
        if 'Name' in device_output_dict:
            device_output_cleaned_dict['name'] = device_output_dict['Name']
        if 'TimeOn' in device_output_dict:
            device_output_cleaned_dict['time_on'] = device_output_dict['TimeOn']
        if 'Power' in device_output_dict:
            device_output_cleaned_dict['power'] = device_output_dict['Power']
        if 'EnergyMax' in device_output_dict:
            device_output_cleaned_dict['energy_max'] = device_output_dict['EnergyMax']
        if 'HourOn' in device_output_dict:
            device_output_cleaned_dict['hour_on'] = device_output_dict['HourOn'] if device_output_dict['HourOn'] != 'xx:xx' else None
        if 'Level' in device_output_dict:
            device_output_cleaned_dict['level'] = device_output_dict['Level']
        if 'Force' in device_output_dict:
            device_output_cleaned_dict['force'] = device_output_dict['Force']
        if 'Control' in device_output_dict:
            device_output_cleaned_dict['control'] = device_output_dict['Control']
        if 'Status' in device_output_dict:
            if type(device_output_dict['Status']) is bool:
                device_output_cleaned_dict['status'] = 1 if device_output_dict['Status'] else 0
            else:
                device_output_cleaned_dict['status'] = device_output_dict['Status']

        # print('FROM DICT ', device_output_cleaned_dict)
        return device_output_cleaned_dict

    @classmethod
    def clean_for_device(cls, output_data, fields=None):
        cleaned_for_device_dict = {}

        if fields is None or 'ID' in fields:
            cleaned_for_device_dict['ID'] = output_data['device_id'].username
        if fields is None or 'Output' in fields:
            cleaned_for_device_dict['Output'] = output_data['output']
        if fields is None or 'Name' in fields:
            cleaned_for_device_dict['Name'] = output_data['name']
        if fields is None or 'TimeOn' in fields:
            cleaned_for_device_dict['TimeOn'] = output_data['time_on']
        if fields is None or 'Power' in fields:
            cleaned_for_device_dict['Power'] = output_data['power']
        if fields is None or 'EnergyMax' in fields:
            cleaned_for_device_dict['EnergyMax'] = output_data['energy_max']
        # REVISAR LA CONDICION PARA EL TIME MAX
        if 'energy_max' in output_data:
            if output_data['power'] is not None and output_data['power'] != 0:
                cleaned_for_device_dict['TimeMax'] = (output_data['energy_max'] * 60000) / output_data['power']
            else:
                cleaned_for_device_dict['TimeMax'] = 0
        if fields is None or 'HourOn' in fields:
            cleaned_for_device_dict['HourOn'] = output_data['hour_on'].strftime('%H:%M') if output_data['hour_on'] is not None else "xx:xx"
        if fields is None or 'Level' in fields:
            cleaned_for_device_dict['Level'] = output_data['level']
        if fields is None or 'Force' in fields:
            cleaned_for_device_dict['Force'] = output_data['force']
        if fields is None or 'Control' in fields:
            cleaned_for_device_dict['Control'] = output_data['control']
        if fields is None or 'Status' in fields:
            cleaned_for_device_dict['Status'] = True if output_data['status'] == cls.STATUS_ON else False

        return cleaned_for_device_dict

    def to_dict(self):
        return {
            'ID': self.device_id.username,
            'Output': self.output,
            'Name': self.name,
            'TimeOn': self.time_on,
            'Power': self.power,
            'EnergyMax': self.energy_max,
            'TimeMax': (self.energy_max * 60000) / self.power if self.power != 0 else 0,
            'HourOn': self.hour_on.strftime('%H:%M') if self.hour_on is not None else "xx:xx",
            'Level': self.level,
            'Force': self.force,
            'Control': self.control,
            'Status': self.status,
        }


from django import forms

class DeviceOutputForm(forms.ModelForm):
    class Meta:
        model = DeviceOutput
        fields = [
            'device_id',
            'output',
            'name',
            'time_on',
            'power',
            'energy_max',
            'hour_on',
            'level',
            'force',
            'control',
            'status',
        ]

    def clean(self):
        cleaned_data = super().clean()
        print('CLEANED DATA ', cleaned_data)
        power = cleaned_data.get('power') or 0
        # print('POWER ', power)
        energy_max = cleaned_data.get('energy_max') or 0
        # print('ENERGY MAX ', energy_max)

        if energy_max != 0:
            if power == 0:
                raise forms.ValidationError('The power consumption must be specified.')
            elif (energy_max * 60000) / power > DeviceOutput.MAX_TIME_MAX:
                raise forms.ValidationError(f'Time max must be less than {DeviceOutput.MAX_TIME_MAX}.')

        # return cleaned_data


class DeviceTemperature(models.Model):
    # device_id = models.ForeignKey(Device, on_delete=models.CASCADE, primary_key=True)
    device_id = models.OneToOneField(settings.AUTH_USER_MODEL, on_delete=models.CASCADE)

    MIN_TEMPERATURE = 0
    MAX_TEMPERATURE = 40

    temperature = RangedFloatField(null=True, blank=False, min_value=MIN_TEMPERATURE, max_value=MAX_TEMPERATURE)

    @classmethod
    def from_dict(cls, device_temperature_dict):
        device_temperature_cleaned_dict = {}
        if 'ID' in device_temperature_dict:
            device_temperature_cleaned_dict['device_id'] = device_temperature_dict['ID']
        if 'SetTemp' in device_temperature_dict:
            device_temperature_cleaned_dict['temperature'] = device_temperature_dict['SetTemp']

        return device_temperature_cleaned_dict

    def to_dict(self):
        return {
            'ID': self.device_id,
            'SetTemp': self.temperature,
        }


class DeviceTemperatureForm(forms.ModelForm):
    class Meta:
        model = DeviceTemperature
        fields = ['device_id', 'temperature']


# class DeviceTemperatures(models.Model):
#     device_id = models.ForeignKey(Device, on_delete=models.CASCADE)

#     heater = models.IntegerField(null=True, blank=True)
#     solar_heater = models.IntegerField(null=True, blank=True)


# la clase abstracta es la que tiene que tener el campo many to many
# class Module(models.Model):
#     device = models.ForeignKey(Device, on_delete=models.CASCADE)

#     class Meta:
#         abstract = True


# class Heater(Module):
#     temperature = RangedFloatField(null=True, blank=False, min_value=-10, max_value=35.5)
    

# class SolarHeater(Module):
#     temperature = RangedFloatField(null=True, blank=False, min_value=-10, max_value=35.5)
