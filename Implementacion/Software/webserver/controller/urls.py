from django.urls import path
from . import views


urlpatterns = [
    path('outputs/manage/<str:device_id>/', views.manage_outputs, name='manage_outputs'),
    path('config/<str:device_id>/', views.get_config, name='get_config'),
    path('user/devices', views.get_devices, name='get_devices'),
    path('user/devices/unlink/<str:device_id>/', views.unlink_device, name='unlink_device'),
    path('dashboard/', views.user_dashboard, name='user_dashboard'),
    path('dashboard/device/<str:device_id>/', views.device_dashboard, name='device_dashboard'),
    path('user/', views.user, name='user'),
    # path('status', views.kit_status_get, name='kit_status_get'),
    # path('temperature', views.temperature, name='temperature')
]
