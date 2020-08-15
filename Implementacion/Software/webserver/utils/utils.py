from django.http import JsonResponse
from django.core.serializers.json import DjangoJSONEncoder
from rest_framework import status


def json_response(body=None, message=None, status=status.HTTP_200_OK, headers=None, safe=True):
    body = body or {}
    headers = headers or {}

    if message is not None:
        body['message'] = message

    response = JsonResponse(body, status=status, safe=safe)

    print(headers)
    for name, value in headers.items():
        response[name] = value

    return response
