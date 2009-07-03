/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Sergey Gridassov
 *
 * Ths program is free software: you can redistribute it and/or modify
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


#ifndef __SPARKLE_NODE__H__
#define __SPARKLE_NODE__H__

#include <QObject>
#include <QHostAddress>

#include "RSAKeyPair.h"
#include "Blowfish.h"


class SparkleNode : public QObject
{
	Q_OBJECT
public:
	SparkleNode(QHostAddress host, quint16 port, QObject *parent = 0);

	virtual ~SparkleNode();

	QHostAddress getHost();
	quint16 getPort();
	void appendQueue(QByteArray data);

	Blowfish *getFromKey() { return &fromKey; }
	Blowfish *getToKey() { return &toKey; }

	RSAKeyPair *getRSA() { return &keyPair; }

	bool isQueueEmpty();
	QByteArray getFromQueue();

	bool keyNegotiationDone;

private:
	QHostAddress host;
	quint16 port;

	QList<QByteArray> queue;
	RSAKeyPair keyPair;
	Blowfish fromKey, toKey;
};

#endif
