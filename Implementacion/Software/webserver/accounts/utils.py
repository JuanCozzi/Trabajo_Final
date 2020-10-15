def check_superuser(user):
    if not user.is_authenticated:
        return False

    return user.is_superuser

def check_device_user(user):
    if not user.is_authenticated:
        return False

    return user.is_device

def check_application_user(user):
    if not user.is_authenticated:
        return False

    return user.is_application_user
