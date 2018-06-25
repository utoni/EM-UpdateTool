#!/usr/bin/env python2.7

from sys import argv
import uuid, json
from bottle import route, run, template, request, response, abort, post

DEFAULT_TEXT = '<html><body><b>default page</b></body></html>'
EMC_VERSION = '1.50'
EMC_SERIAL = '0123456789'
PASSWORD = None
EMC_OK = json.dumps({'app_version': EMC_VERSION , 'serial': EMC_SERIAL , 'authentication': True}, sort_keys=True, indent=4, separators=(',', ': '))
EMC_AUTH = json.dumps({'app_version': EMC_VERSION , 'serial': EMC_SERIAL , 'authentication': False}, sort_keys=True, indent=4, separators=(',', ': '))


@route('/start.php', method='GET')
def get_start_php():
    sid = uuid.uuid1()
    response.set_header('Set-Cookie', 'PHPSESSID='+str(sid))
    print 'PHPSESSID:', str(sid)
    return EMC_AUTH

@route('/start.php', method='POST')
def post_start_php():
    lines = request.body.readlines()
    print 'Request BODY:', '"'+str(lines[0])+'"'

    passwd = ''
    try:
        passwd = request.forms.get('password')
    except KeyError:
        pass
    if PASSWORD is not None:
        if passwd != PASSWORD:
            print 'Auth failed: "%s" != "%s"' % (PASSWORD, passwd)
            return EMC_AUTH

    return EMC_OK

@route('/setup.php', method='GET')
def get_setup_php():
    cleanup = None
    try:
        cleanup = request.query['update_cleanup']
    except KeyError:
        pass

    if cleanup is None:
        abort(404)

    return EMC_OK

@route('/mum-webservice/0/update.php', method='POST')
def post_setup_php():
    upload = request.files.get('update_file')
    upload_len = len(upload.file.read())
    print 'Update ... Length:', str(upload_len), 'bytes'
    return DEFAULT_TEXT


if __name__ == '__main__':
    try:
        PASSWORD = argv[1]
    except IndexError:
        pass
    print 'Device Password:', PASSWORD
    try:
        listen_adr = argv[2]
    except IndexError:
        listen_adr = '127.0.0.1'
    try:
        listen_port = argv[3]
    except IndexError:
        listen_port = 80
    print 'Listen Address:', listen_adr + ':' + str(listen_port)
    run(host=listen_adr, port=listen_port)
