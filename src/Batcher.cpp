/*
 * This file is part of ePipe
 * Copyright (C) 2019, Logical Clocks AB. All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Batcher.h"

Batcher::Batcher(const int time_to_wait, const int batch_size)
: mBatchSize(batch_size), mTimerProcessing(false), mTimeToWait(time_to_wait),
mStarted(false), mFirstTimer(true), mScheduleShutdown(false), mShutdownTimer(false) {
  srand(time(NULL));
}

void Batcher::start() {
  if (mStarted) {
    LOG_INFO("Batcher is already started");
    return;
  }

  startTimer();
  mThread = boost::thread(&Batcher::run, this);
  mStarted = true;
}

void Batcher::waitToFinish() {
  if (mStarted) {
    mThread.join();
    mTimerThread.join();
  }
}

void Batcher::shutdown(){
  if(mStarted){
    LOG_INFO("Shutting down batcher timer..");
    mScheduleShutdown = true;
  }
}

void Batcher::startTimer() {
  LOG_DEBUG("start timer");
  mTimerThread = boost::thread(&Batcher::timerThread, this);
}

void Batcher::timerThread() {
  while (true) {
    if(mShutdownTimer){
      LOG_INFO("Shutdown batcher timer");
      break;
    }
    mIOService.reset();
    int timeout = mTimeToWait;
    if (mFirstTimer) {
      int baseTime = mTimeToWait / 4;
      timeout = rand() % (mTimeToWait - baseTime) + baseTime;
      mFirstTimer = false;
      LOG_TRACE("fire the first timer after " << timeout << " msec");
    }
    boost::asio::deadline_timer timer(mIOService, boost::posix_time::milliseconds(timeout));
    timer.async_wait(boost::bind(&Batcher::timerExpired, this, boost::asio::placeholders::error));
    mIOService.run();
  }
}

void Batcher::resetTimer() {
  mIOService.stop();
}

void Batcher::timerExpired(const boost::system::error_code& e) {
  if (e) return;
  mTimerProcessing = true;
  processBatch();
  mTimerProcessing = false;
  if(mScheduleShutdown){
    mShutdownTimer = true;
  }
}

Batcher::~Batcher() {
}

