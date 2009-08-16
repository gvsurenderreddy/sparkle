/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Sergey Gridassov, Peter Zotov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtDebug>
#include <openssl/rand.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

#include "RSAKeyPair.h"
#include "ArgumentParser.h"
#include "LinkLayer.h"
#include "UdpPacketTransport.h"
#include "Log.h"
#include "Router.h"

#ifdef Q_WS_X11
#include "LinuxTAP.h"
#endif

QHostAddress checkoutAddress(QString strAddr) {
	QHostAddress ipAddr;
	if(!ipAddr.setAddress(strAddr)) {
		QHostInfo hostInfo = QHostInfo::fromName(strAddr);
		if(hostInfo.error() != QHostInfo::NoError) {
			Log::warn("cannot lookup address for host %1") << strAddr;
		} else {
			QList<QHostAddress> list = hostInfo.addresses();
			if(list.size() > 1)
				Log::warn("there are more than one IP address for host %1, using first (%2)")
						<< strAddr << list[0].toString();
			
			return list[0];
		}
	}
	return ipAddr;
}

int main(int argc, char *argv[]) {
	QCoreApplication app(argc, argv);
	app.setApplicationName("sparkle");

	QString profile = "default", configDir;
	bool createNetwork = false, noTap = false;
	QHostAddress localAddress = QHostAddress::Any, remoteAddress;
	quint16 localPort = 1801, remotePort = 1801;

	int keyLength = 1024;
	bool generateNewKeypair = false;
	
	qsrand(QDateTime::currentDateTime().toTime_t());

	{
		QString createStr, joinStr, bindStr, keyLenStr, getPubkeyStr, noTapStr;

		ArgumentParser parser(app.arguments());

		parser.registerOption(QChar::Null, "profile", ArgumentParser::RequiredArgument, &profile, NULL,
			NULL, "use specified profile", "PROFILE");

		parser.registerOption('c', "create", ArgumentParser::NoArgument, &createStr, NULL,
			NULL, "\tcreate new network", NULL);

		parser.registerOption('j', "join", ArgumentParser::RequiredArgument, &joinStr, NULL,
			NULL, "\n\t\t\tjoin existing network, PORT defaults to 1801", "HOST[:PORT]");

		parser.registerOption('b', "bind", ArgumentParser::RequiredArgument, &bindStr, NULL,
			NULL, "\n\t\t\tbind to local UDP endpoint HOST:PORT, defaults to *:1801", "HOST[:PORT]");

		parser.registerOption(QChar::Null, "generate-key", ArgumentParser::RequiredArgument,
			&keyLenStr, NULL, NULL, "generate new RSA key pair with specified length", "BITS");

		parser.registerOption(QChar::Null, "get-pubkey", ArgumentParser::NoArgument,
			&getPubkeyStr, NULL, NULL, "\tprint my public key", NULL);
		
		parser.registerOption(QChar::Null, "no-tap", ArgumentParser::NoArgument,
			&noTapStr, NULL, NULL, "\tdo not create TAP interface (`headless' mode)", NULL);
		
		if(!parser.parse()) { // help was displayed
			return 0;
		}
		
		configDir = QDir::homePath() + "/." + app.applicationName() + "/" + profile;
		QDir().mkdir(QDir::homePath() + "/." + app.applicationName());
		QDir().mkdir(configDir);

		if(!getPubkeyStr.isNull()) {
			RSAKeyPair keyPair;
	
			if(!keyPair.readFromFile(configDir + "/rsa_key")) {
				Log::fatal("cannot read RSA keypair");
				return 1;
			} else {
				printf("%s", QString(keyPair.getPublicKey()).toLocal8Bit().constData());
				return 0;
			}
		}

		if(!createStr.isNull() && !joinStr.isNull()) {
			Log::fatal("options --create and --join cannot be specified simultaneously");
			return 1;
		}
		
		if(createStr.isNull() && joinStr.isNull()) {
			Log::fatal("specify --create or --join option, not both");
			return 1;
		}
		
		if(!createStr.isNull())
			createNetwork = true;
		
		if(!joinStr.isNull()) {
			QStringList parts = joinStr.split(":");

			remoteAddress = checkoutAddress(parts[0]);
			if(remoteAddress.isNull()) {
				Log::fatal("invalid node address %1") << parts[0];
				return 1;
			}
			
			if(parts.size() == 1) {
				/* already assigned */
			} else if(parts.size() == 2) {
				remotePort = parts[1].toInt();
			} else {
				Log::fatal("invalid node address %1") << joinStr;
				return 1;
			}
		}
		
		if(!bindStr.isNull()) {
			QStringList parts = bindStr.split(":");

			if(parts[0] == "*") {
				localAddress = QHostAddress::Any;
			} else {
				localAddress = checkoutAddress(parts[0]);
				if(localAddress.isNull()) {
					Log::fatal("invalid address %1") << parts[0];
					return 1;
				}
			}
		
			if(parts.size() == 1) {
			} else if(parts.size() == 2) {
				localPort = parts[1].toInt();
			} else {
				Log::fatal("invalid endpoint %1") << joinStr;
				return 1;
			}
		}
		
		if(!keyLenStr.isNull()) {
			generateNewKeypair = true;
			keyLength = keyLenStr.toInt();
		}
		
		if(createNetwork && localAddress == QHostAddress::Any) {
			Log::fatal("you need to specify local endpoint to create network");
			return 1;
		}
		
		if(!noTapStr.isNull())
			noTap = true;
	}

	RSAKeyPair hostPair;
	
	if(!QFile::exists(configDir + "/rsa_key") || generateNewKeypair) {
		Log::debug("generating new RSA key pair (%1 bits)") << keyLength;

		if(!hostPair.generate(keyLength)) {
			Log::fatal("cannot generate new keypair");
			return 1;
		}

		if(!hostPair.writeToFile(configDir + "/rsa_key")) {
			Log::fatal("cannot write new keypair");
			return 1;
		}
	} else {
		if(!hostPair.readFromFile(configDir + "/rsa_key")) {
			Log::fatal("cannot read RSA keypair");
			return 1;
		}
	}
	
	Router router;
	UdpPacketTransport transport(localAddress, localPort);
	LinkLayer linkLayer(router, transport, hostPair);

	if(!noTap) {
#ifdef Q_WS_X11
		LinuxTAP tap(linkLayer);
		if(tap.createInterface("sparkle%d") == false) {
			Log::fatal("cannot initialize TAP");
			return 1;
		}
#endif
	} else {
		Log::debug("tap: no interface created");
	}

	if(createNetwork) {
		if(!linkLayer.createNetwork(localAddress)) {
			Log::fatal("cannot create network");
			return 1;
		}
	} else {
		if(!linkLayer.joinNetwork(remoteAddress, remotePort)) {
			Log::fatal("cannot join network");
			return 1;
		}
	}

	return app.exec();
}

