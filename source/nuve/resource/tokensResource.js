/*global exports, require, Buffer*/
'use strict';

var dataAccess = require('../data_access');
var crypto = require('crypto');
var cloudHandler = require('../cloudHandler');
var logger = require('./../logger').logger;
var e = require('../errors');

// Logger
var log = logger.getLogger('TokensResource');

var getTokenString = function (id, token) {
    return dataAccess.token.key().then(function(nuveKey) {
        var toSign = id + ',' + token.host,
            hex = crypto.createHmac('sha256', nuveKey).update(toSign).digest('hex'),
            signed = (new Buffer(hex)).toString('base64'),

            tokenJ = {
                tokenId: id,
                host: token.host,
                secure: token.secure,
                signature: signed
            },
            tokenS = (new Buffer(JSON.stringify(tokenJ))).toString('base64');

        return tokenS;

    }).catch(function(err) {
        log.error('Get nuveKey error:', err);
    });
};

/*
 * Generates new token. 
 * The format of a token is:
 * {tokenId: id, host: erizoController host, signature: signature of the token};
 */
var generateToken = function (currentRoom, authData, origin, callback) {
    var currentService = authData.service,
        user = authData.user,
        role = authData.role,
        r,
        tr,
        token,
        tokenS;

    if (user === undefined || user === '') {
        callback(undefined);
        return;
    }

    if (!authData.room.roles.find((r) => (r.role === role))) {
        callback(undefined);
        return;
    }

    token = {};
    token.user = user;
    token.room = currentRoom;
    token.role = role;
    token.service = currentService._id;
    token.creationDate = new Date();
    token.origin = origin;
    token.code = Math.floor(Math.random() * 100000000000) + '';

    // Values to be filled from the erizoController
    token.secure = false;

    cloudHandler.schedulePortal (token.code, origin, function (ec) {
        if (ec === 'timeout') {
            callback('error');
            return;
        }

        token.secure = ec.ssl;
        if (ec.hostname !== '') {
            token.host = ec.hostname;
        } else {
            token.host = ec.ip;
        }

        token.host += ':' + ec.port;

        dataAccess.token.create(token, function(id) {
            getTokenString(id, token)
                .then((tokenS) => {
                    callback(tokenS);
                });
        });
    });
};

/*
 * Post Token. Creates a new token for a determined room of a service.
 */
exports.create = function (req, res, next) {
    var authData = req.authData;
    authData.user = (req.authData.user || (req.body && req.body.user));
    authData.role = (req.authData.role || (req.body && req.body.role));
    var origin = ((req.body && req.body.preference) || {isp: 'isp', region: 'region'});

    generateToken(req.params.room, authData, origin, function (tokenS) {
        if (tokenS === undefined) {
            log.info('Name and role?');
            return next(new e.BadRequestError('Name or role not valid'));
        }
        if (tokenS === 'error') {
            log.info('CloudHandler does not respond');
            return next(new e.CloudError('Failed to get portal'));
        }
        log.debug('Created token for room ', req.params.room, 'and service ', authData.service._id);
        res.send(tokenS);
    });
};

