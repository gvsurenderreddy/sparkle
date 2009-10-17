/*
 * Sippy - zero-configuration fully distributed self-organizing encrypting IM
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

#include <QtDebug>
#include <QSocketNotifier>

#include "DebugConsole.h"

DebugConsole::DebugConsole(QWidget *parent) : QDialog(parent) {
	setupUi(this);

	debugOutput->setFontFamily("Monospace");

	pipe(outputPipe);

//	stdout = fdopen(outputPipe[1], "a");
//	stderr = stdout;

	FILE *tmp = fdopen(outputPipe[1], "a");

	if(tmp == NULL)
		perror("fdopen");

	setlinebuf(tmp);

	pipeNotify = new QSocketNotifier(outputPipe[0], QSocketNotifier::Read, this);
	connect(pipeNotify, SIGNAL(activated(int)), SLOT(pipeReadable()));

	fclose(stdout);
	fclose(stderr);

	stdout = tmp;
	stderr = tmp;
}

DebugConsole::~DebugConsole() {
	::close(outputPipe[0]);
	::close(outputPipe[1]);
}

void DebugConsole::pipeReadable() {
	char buf[256];

	int readed = read(outputPipe[0], buf, sizeof(buf) - 1);

	if(readed == -1) {
		perror("read");

		pipeNotify->setEnabled(false);

		return;
	}

	buf[readed] = 0;

	QString text = debugOutput->toPlainText();

	text += QString::fromLocal8Bit(buf);

	debugOutput->setPlainText(text);
}

