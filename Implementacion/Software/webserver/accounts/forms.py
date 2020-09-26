from django.contrib.auth.forms import UserCreationForm
# from django.contrib.auth.models import User
from django import forms
from django.utils.translation import ugettext_lazy as _
from django.db import transaction

from django.contrib.auth.validators import UnicodeUsernameValidator

from .models import User, Device



class CreateApplicationUserForm(UserCreationForm):
	username_validator = UnicodeUsernameValidator()
	
	email = forms.EmailField(required=True)
	device_id = forms.CharField(
        max_length=150,
        required=True,
        help_text=_('Required. 150 characters or fewer. Letters, digits and @/./+/-/_ only.'),
        validators=[username_validator]
    )
	
	class Meta:
		model = User
		fields = ['username', 'email', 'device_id', 'password1', 'password2']
		# fields = ['email', 'password1', 'password2']

	def clean_device_id(self):
		device_id = self.cleaned_data['device_id']
		invalid_device_id = False
		try:
			device = User.objects.get(username=device_id)
			if not device.is_device:
				invalid_device_id = True
			else:
				# checks the device is not assigned to another user
				invalid_device_id = Device.objects.filter(device=device).exists()
		except User.DoesNotExist:
			invalid_device_id = True



		if invalid_device_id:
			raise forms.ValidationError(_('Invalid device: "%(device_id)s".'),
										params={'device_id': device_id},
										code='invalid')

		return device_id

	@transaction.atomic
	def save(self, device_name=None):
		user = super().save(commit=False)
		user.is_application_user = True
		user.email = self.cleaned_data.get('email')
		user.save()
		print('Device id ', self.cleaned_data.get('device_id'))
		Device.objects.create(user=user, device=User.objects.get(username=self.cleaned_data.get('device_id')), name=device_name)
		
		return user


class CreateDeviceForm(UserCreationForm):

	class Meta:
		model = User
		fields = ['username', 'password1', 'password2']

	def save(self, commit=True):
		user = super().save(commit=False)
		user.is_device = True
		
		if commit:
			user.save()

		return user
