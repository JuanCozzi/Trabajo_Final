from django.db import models
from django.contrib.auth.models import AbstractUser, AbstractBaseUser, PermissionsMixin
from django.utils.translation import ugettext_lazy as _
from django.utils import timezone

from django.conf import settings
from django.db.models.signals import post_save
from django.dispatch import receiver
from rest_framework.authtoken.models import Token

from .managers import CustomUserManager

from django.contrib.auth.validators import UnicodeUsernameValidator


# delete db.sqlite3
# delete migrations folder in app
# python manage.py makemigrations accounts
# python manage.py migrate
# python manage.py dbshell
# .schema <TABLE>

class User(AbstractUser, PermissionsMixin):
    username_validator = UnicodeUsernameValidator()

    username = models.CharField(
        _('username'),
        max_length=150,
        unique=True,
        help_text=_('Required. 150 characters or fewer. Letters, digits and @/./+/-/_ only.'),
        validators=[username_validator],
        error_messages={
            'unique': _("A user with that username already exists."),
        },
    )

    email = models.EmailField(_('Email address'), unique=True, null=True)

    is_application_user = models.BooleanField(default=False)
    is_device = models.BooleanField(default=False)

    # name = models.CharField(max_length=254, null=True, blank=True)
    is_staff = models.BooleanField(default=False)
    is_superuser = models.BooleanField(default=False)
    is_active = models.BooleanField(default=True)
    last_login = models.DateTimeField(default=timezone.now)
    date_joined = models.DateTimeField(default=timezone.now)

    # USERNAME_FIELD = 'email'
    USERNAME_FIELD = 'username'
    REQUIRED_FIELDS = []

    objects = CustomUserManager()

    def save(self, *args, **kwargs):
        if self.email == '':
            print('Saving user ', type(self.email))
            self.email = None
        super(User, self).save(*args, **kwargs)

    def __str__(self):
        user_type = 'Admin'
        if self.is_device:
            user_type = 'Device'
        elif self.is_application_user:
            user_type = 'User'

        return f'{user_type} {self.username}'


class Device(models.Model):
    device = models.OneToOneField(User, on_delete=models.CASCADE, related_name='device', primary_key=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE, related_name='user')
    name = models.CharField(_('Descriptive name'), max_length=150, null=True)
    connected = models.BooleanField(default=False)
    unlink_required = models.BooleanField(default=False)

    @classmethod
    def set_connected(cls, device, connected):
        device_user = cls.objects.get(device=device)
        device_user.connected = connected
        device_user.save()
    
    @classmethod
    def set_unlink_required(cls, user, device):
        device_user = cls.objects.get(user=user, device=device)
        device_user.unlink_required = True
        device_user.save()


# Para mover esto a un signals.py hay que asociarlo en admin.py y settings.py
@receiver(post_save, sender=settings.AUTH_USER_MODEL)
def create_auth_token(sender, instance=None, created=False, **kwargs):
    if created:
        Token.objects.create(user=instance)
