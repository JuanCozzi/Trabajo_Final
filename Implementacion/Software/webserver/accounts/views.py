from django.shortcuts import render, redirect
from django.contrib.auth import authenticate, login as auth_login, logout as auth_logout
from django.db.utils import IntegrityError
from rest_framework import status

# from .models import *
from utils.utils import json_response
from accounts.models import User, Device
from .forms import CreateApplicationUserForm, CreateDeviceForm
from .decorators import unauthenticated_user

from controller.views import user_dashboard

import json
# {
#     "username": "asd",
#     "email": "asd@asd.com",
#     "password1": "sfgsergrthhsgersg453",
#     "password2": "sfgsergrthhsgersg453"
# }

from django.views.decorators.csrf import csrf_exempt



@csrf_exempt
# ESTO LO VA A EJECUTAR EL DISPOSITIVO, ASI QUE PUEDO PEDIR QUE ESTE AUTENTICADO
def register_user(request):
    if request.method == 'POST':
        data = json.loads(request.body)
        # data = request.POST
        print(data)
        data['device_id'] = request.user.username
        user_password = data.pop('password')
        data['password1'] = user_password
        data['password2'] = user_password
        print('Data previous to form ', data)
        # raise Exception('ACA')
        if User.objects.filter(username=data['username']).exists():
            print('Ya existe el usuario, agrego el dispositivo')
            user = User.objects.get(username=data['username'])
            # HAY QUE VALIDAR EL ID DEL DEVICE
            try:
                Device.objects.create(user=user, device=User.objects.get(username=data['device_id']), name=data['device_name'])
                return json_response(message=f'Now user {user.username} ' \
                                        f'with email {user.email} is related to device {request.user.username}')
            except IntegrityError:
                return json_response(message=f'Device registered for another user', status=status.HTTP_400_BAD_REQUEST)
        else:
            application_user_form = CreateApplicationUserForm(data)
            print(application_user_form.errors)
            if application_user_form.is_valid():
                # innecesario guardarlo en la variable user me parece
                # try:
                application_user = application_user_form.save(device_name=data['device_name'])
                # LA EXCEPCION NO APLICA, LO ESTOY RESOLVIENDO EN EL FORMULARIO EL CASO QUE EL DISPOSITIVO YA ESTE ASOCIADO A OTRO USUARIO
                # except IntegrityError:
                    # Falló porque el dispositivo ya está asociado a otro usuario
                    # Revelar la información? O dar un mensaje de error sin el por qué
                    # return json_response(message=f'Device registered for another user', status=status.HTTP_400_BAD_REQUEST)

                return json_response(message=f'Created user {application_user.username} ' \
                                    f'with email {application_user.email}, related to device {request.user.username}')
        print(application_user_form.errors.as_json())
        print(application_user_form.errors.get_json_data())
        # quizá hace falta setear escape_html=True en get_json_data
        errors = application_user_form.errors.get_json_data()
        response = {field.capitalize() if field != '__all__' else 'General': errors[0]['message'] for field, errors in errors.items()}
        # response = errors
        return json_response(body=response, status=status.HTTP_400_BAD_REQUEST)

    return json_response(status=status.HTTP_404_NOT_FOUND)

# from rest_framework.decorators import api_view
# In DRF only authenticated requests require CSRF tokens, and anonymous requests may be sent without CSRF tokens.
# This behaviour is not suitable for login views, which should always have CSRF validation applied.
# si uso api_view() puedo usar @method_decorator(csrf_protect)
# CSRF es problema del browser, no de la aplicación
@csrf_exempt
@unauthenticated_user
def login(request):
    if request.method == 'POST':
        print('Data ', request.body)
        data = json.loads(request.body)
        print('Data ', data)
        user = authenticate(request, username=data['username'], password=data['password'])
        print(user)
        if user is not None:
            auth_login(request, user)
            # return json_response(data={'token': Token.objects.get(user=user).key}) 
            return json_response(message='User successfully authenticated')            

        return json_response(status=status.HTTP_401_UNAUTHORIZED)
    
    return json_response(status=status.HTTP_404_NOT_FOUND)

@csrf_exempt
def logout(request):
    auth_logout(request)
    return json_response()

@csrf_exempt
# ESTO RESTRINGIRLO A ADMINS
def register_device(request):
    if request.method == 'POST':
        data = json.loads(request.body)
        # data = request.POST
        print(data)
        device_form = CreateDeviceForm(data)
        if device_form.is_valid():
            # innecesario guardarlo en la variable user me parece
            device = device_form.save()            

            return json_response(message=f'Created device {device.username} with email {device.email}')
        print(device_form.errors.as_json())
        print(device_form.errors.get_json_data())
        # quizá hace falta setear escape_html=True en get_json_data
        return json_response(body=device_form.errors.get_json_data(), status=status.HTTP_400_BAD_REQUEST)

    return json_response(status=status.HTTP_404_NOT_FOUND)

def home(request):
    return render(None, 'accounts/sign_up_login.html')
