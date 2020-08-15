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
device = save_form(form)
form = CreateApplicationUserForm({'username': 'pau1', 'email': 'a@a.com', 'device_id': 'pd1', 'password1': 'qwertyas', 'password2': 'qwertyas'})
save_form(form)

from controller.models import DeviceOutputForm, DeviceTemperatureForm

form = DeviceOutputForm({'device_id': device, 'output': 1})
save_form(form)
# form = DeviceTemperatureForm({'device_id': device, 'temperature': 25})
# save_form(form)
