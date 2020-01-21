#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import web
import re
import base64
import json
import pickle
import time

URLS = (
    '/minisw/?', 'minisw'
)

allowed = (
    ('iot1','passwd1'),
    ('iot2','passwd2')
)

web.config.debug = False

DB_FILE = 'minisw.db'

class minisw:
    def update(self, args):
        try:
            f = open(DB_FILE, 'wb')
            print(args)
            act = int(args.act)
            pc = int(args.pc)
            ts = str(int(time.time()))
            if pc == 1:
                kv = {'pc':pc, 'act':act, 'ts':ts}
                pickle.dump(kv, f)
                return 'succ'
        except Exception as e:
            print(e)
        finally:
            if f:
                f.close()
        return 'err'

    def pick(self):
        try:
            f = open(DB_FILE, 'rb')
            kv = pickle.load(f)
            print(kv)
            act = kv['act']
            pc = kv['pc']
            ts = kv['ts']
            rsp = {'pc':pc, 'act':act, 'ts':ts}
            return str(json.dumps(rsp))
        except Exception as e:
            print(e)
        finally:
            if f:
                f.close()
        return 'err'

    def accept(self, args):
        print(args)
        if len(args.keys()) == 0:
            return self.pick()
        return self.update(args)

    def GET(self):
        auth = web.ctx.env.get('HTTP_AUTHORIZATION')
        authreq = False
        if auth is None:
            authreq = True
        else:
            auth = re.sub('^Basic ','',auth)
            username,password = str(base64.b64decode(auth), 'utf-8').split(':')
            if (username,password) in allowed:
                return self.accept(web.input())
            else:
                authreq = True
        if authreq:
            web.header('WWW-Authenticate','Basic realm="Auth example"')
            web.ctx.status = '401 Unauthorized'
            return

    def POST(self):
        return '0'

def main():
    app = web.application(URLS, globals())
    app.run()

if __name__ == '__main__':
    main()
