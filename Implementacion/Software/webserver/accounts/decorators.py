from rest_framework import status

from utils.utils import json_response

from enum import Enum


class UserRole(Enum):
    APPLICATION_USER = 1
    DEVICE = 2
    SUPERUSER = 3


def unauthenticated_user(view_func):
    def wrapper_func(request, *args, **kwargs):
        print('Decorator ', request.user)
        if request.user.is_authenticated:
            return json_response(message='User already authenticated')
        else:
            return view_func(request, *args, **kwargs)

    return wrapper_func

def check_user(user_role):
    def decorator(view_func):
        def wrapper_func(request, *args, **kwargs):
            if not request.user.is_authenticated:
                return json_response(message='User must be authenticated', status=status.HTTP_401_UNAUTHORIZED)

            if user_role == UserRole.APPLICATION_USER and request.user.is_application_user \
                or user_role == UserRole.DEVICE and request.user.is_device \
                or user_role == UserRole.SUPERUSER and request.user.is_superuser:
                return view_func(request, *args, **kwargs)
            else:
                return json_response(message='User not authorized', status=status.HTTP_403_FORBIDDEN)

        return wrapper_func
    return decorator

# también existe user_passes_test() que recibe una función y listo
def allowed_users(allowed_roles=None):
    def decorator(view_func):
        def wrapper_func(request, *args, **kwargs):
            if not request.user.is_authenticated:
                return json_response(message='User must be authenticated', status=status.HTTP_401_UNAUTHORIZED)

            if request.user.groups.filter(name__in=allowed_roles).exists():
                return view_func(request, *args, **kwargs)
            else:
                return json_response(message='User not authorized', status=status.HTTP_403_FORBIDDEN)

        return wrapper_func
    return decorator
