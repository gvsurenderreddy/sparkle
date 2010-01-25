/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Sergey Gridassov
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

#ifndef __ETHERNET_APPLICATION_LAYER__H__
#define __ETHERNET_APPLICATION_LAYER__H__

#include <QObject>
#include <QHostAddress>
#include <SparkleAddress.h>
#include <ApplicationLayer.h>

class Router;
class LinkLayer;
class TapInterface;
class SparkleNode;

class EthernetApplicationLayer: public QObject, public ApplicationLayer {
	Q_OBJECT

public:
	EthernetApplicationLayer(LinkLayer &linkLayer, TapInterface* tap);
	virtual ~EthernetApplicationLayer();

	virtual void handleDataPacket(QByteArray &packet, SparkleAddress address);

private slots:
	void haveTapPacket(QByteArray packet);
	void initialize(SparkleNode* self);

signals:
	void sendTapPacket(QByteArray packet);

private:
	void sendARPReply(SparkleAddress address);

	static QHostAddress makeIPv4Address(SparkleAddress mac);

private:
	struct ethernet_header_t {
		quint8		dest[6];
		quint8		src[6];
		quint16		type;
	} __attribute__((packed));

	struct arp_packet_t {
		quint16		htype;
		quint16		ptype;
		quint8		hlen;
		quint8		plen;
		quint16		oper;
		quint8		sha[6];
		quint32		spa;
		quint8		tha[6];
		quint32		tpa;
	} __attribute__((packed));

	struct ipv4_header_t {
		quint8		version;
		quint8		diffserv;
		quint16		size;
		quint16		id;
		quint16		fragments;
		quint8 		ttl;
		quint8 		protocol;
		quint16		checksum;
		quint32		src;
		quint32		dest;
	} __attribute__((packed));

private:
	Router &router;
	LinkLayer &linkLayer;
	TapInterface* tap;

	SparkleAddress selfMAC;
	QHostAddress selfIPv4;
};

#endif

