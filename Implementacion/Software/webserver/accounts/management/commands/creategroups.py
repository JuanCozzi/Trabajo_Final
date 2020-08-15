from django.core.management.base import BaseCommand
from django.apps import apps


class Command(BaseCommand):
    help = 'Create application user groups'

    def add_arguments(self, parser):
        parser.add_argument('groups', nargs='+', type=str)

    def handle(self, *args, **options):
        Group = apps.get_model('auth', 'Group')
        for group_name in options['groups']:
            group, created = Group.objects.get_or_create(name=group_name)
            if created:
                group.save()
                self.stdout.write(self.style.SUCCESS(f'Successfully created group {group_name}'))
            else:
                self.stdout.write(self.style.SUCCESS(f'Group {group_name} already exists'))
