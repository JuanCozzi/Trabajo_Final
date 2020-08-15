from django.urls import path
from django.contrib.auth import views as auth_views
from . import views


urlpatterns = [
    path('', views.home, name='home'),
    path('register/user/', views.register_user, name='register_user'),
    path('register/device/', views.register_device, name='register_device'),
    path('login/', views.login, name='login'),
    path('logout/', views.logout, name='logout'),
    # path('status', views.kit_status_get, name='kit_status_get'),
    # path('temperature', views.temperature, name='temperature')

    path('password/reset/',
         auth_views.PasswordResetView.as_view(template_name='accounts/password_reset.html'),
         name='reset_password'),

    path('password/reset/sent/',
         auth_views.PasswordResetDoneView.as_view(template_name='accounts/password_reset_sent.html'),
         name='password_reset_done'),

    path('password/reset/<uidb64>/<token>/',
         auth_views.PasswordResetConfirmView.as_view(template_name='accounts/password_reset_confirm.html'),
         name='password_reset_confirm'),

    path('password/reset/complete',
         auth_views.PasswordResetCompleteView.as_view(template_name='accounts/password_reset_complete.html'),
         name='password_reset_complete'),
]
