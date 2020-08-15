rm db.sqlite3
rm -r ./accounts/migrations
rm -r ./controller/migrations

python manage.py makemigrations accounts
python manage.py makemigrations controller
python manage.py migrate

# # creates the user groups
# python manage.py creategroups owner tenant

# creates the superuser (by default it seems that all groups are associated to it)
# echo "from django.contrib.auth import get_user_model; User = get_user_model(); superuser = User.objects.create_superuser('admin', 'admin@admin.com', 'a'); from controller.models import Device, DeviceOutputForm, DeviceTemperatureForm; Device.objects.create(device_id=0, user=superuser); form = DeviceOutputForm({'device_id': 0, 'output': 1}); print('ERRORS ', form.errors.get_json_data()); form.save(); form = DeviceTemperatureForm({'device_id': 0, 'temperature': 25}); print('ERRORS ', form.errors.get_json_data()); form.save();" | python manage.py shell
cat .\setup_db.py | python manage.py shell
