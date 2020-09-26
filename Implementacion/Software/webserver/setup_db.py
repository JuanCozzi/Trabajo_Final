def save_form(form):
    if form.is_valid():
        return form.save()
    else:
        print('ERRORS ', form.errors.get_json_data())
        return None

from django.contrib.auth import get_user_model

User = get_user_model()
superuser = User.objects.create_superuser('admin', 'admin@admin.com', 'a')

from accounts.forms import *

form = CreateDeviceForm({'username': 'pd1', 'password1': 'qwertyas', 'password2': 'qwertyas'})
device_1 = save_form(form)
form = CreateDeviceForm({'username': 'tyWj5hd34k', 'password1': 'qwertyas', 'password2': 'qwertyas'})
device_2 = save_form(form)
form = CreateApplicationUserForm({'username': 'pau1', 'email': 'a@a.com', 'device_id': 'pd1', 'password1': 'qwertyas', 'password2': 'qwertyas'})
save_form(form)
form = CreateApplicationUserForm({'username': 'pau2', 'email': 'a@a.com', 'device_id': 'tyWj5hd34k', 'password1': 'qwertyas', 'password2': 'qwertyas'})
save_form(form)

from controller.models import DeviceOutput, DeviceOutputForm, DeviceTemperatureForm

form = DeviceOutputForm({'device_id': device_1, 'output': 1})
save_form(form)
for i in range(2, DeviceOutput.MAX_OUTPUTS + 1):
    form = DeviceOutputForm({'device_id': device_1, 'output': i})
    save_form(form)
form = DeviceOutputForm({'device_id': device_2, 'output': 1})
save_form(form)
# form = DeviceTemperatureForm({'device_id': device, 'temperature': 25})
# save_form(form)
