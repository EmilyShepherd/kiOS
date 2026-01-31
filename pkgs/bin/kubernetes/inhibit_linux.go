//go:build linux
// +build linux

/*
Copyright 2021 The Kubernetes Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package systemd

import (
	"net"
	"time"

	"k8s.io/klog/v2"
)

type Inhibitor struct {
	isShuttingDown bool
	conn           net.Conn
}

const (
	systemSocket = "/run/system.sock"

	cmdShutdown         = byte(1)
	cmdContinueShutdown = byte(2)

	eventShutdown = byte(1)
)

func NewDBusCon() (*Inhibitor, error) {
	conn, err := net.Dial("unix", systemSocket)
	if err != nil {
		return nil, err
	}

	return &Inhibitor{
		conn: conn,
	}, nil
}

type InhibitLock uint16

// The NodeShutdownManager checks the current system setting for grace.
// kiOS is designed to defer all pod grace decisions to kubelet itself,
// so we just set an absurdly high figure to keep the kubelet happy.
func (bus *Inhibitor) CurrentInhibitDelay() (time.Duration, error) {
	duration := time.Duration(1000) * time.Second
	return duration, nil
}

// Under systemd, the NodeShutdownManager needs to "request" an inhibit
// lock. kiOS was designed with the expectation of a kubelet-inhibited
// shutdown in mind, so does not require an inhibit mechanism.
//
// This will always return an "InhibitLock" in line with the method
// signature, but this is effectively worthless and is not checked again
// anywhere.
func (bus *Inhibitor) InhibitShutdown() (InhibitLock, error) {
	return InhibitLock(0), nil
}

// Called by the NodeShutdownManager when it has finished killing all
// pods and is ready for the system to continue shutting down. This
// notifies the system socket to continue the system shutdown.
//
// (NB: Looks like this is called also when refreshing the lock, so make
// sure you do NOT kick off a shutdown without checking one is in
// progress first!
func (bus *Inhibitor) ReleaseInhibitLock(lock InhibitLock) error {
	if bus.isShuttingDown {
		var cmd = []byte{cmdContinueShutdown}
		bus.conn.Write(cmd)
	}
	return nil
}

// This listens on the system socket for shutdown events. If one is
// received, this will trigger kubelet's Graceful Node Shutdown
// behaviour and kill all the pods in a graceful way.
func (bus *Inhibitor) MonitorShutdown(_ klog.Logger) (<-chan bool, error) {
	shutdownChan := make(chan bool, 1)

	go func() {
		data := make([]byte, 1)
		for {
			bus.conn.Read(data)
			if data[0] == eventShutdown {
				bus.isShuttingDown = true
				shutdownChan <- bus.isShuttingDown
			}
		}
	}()

	return shutdownChan, nil
}

// Called as part of the NodeShutdownManager's init system if it
// believes that the current system shutdown grace period is too low.
//
// No-op as kiOS will always give the kubelet as long as it needs.
func (bus *Inhibitor) OverrideInhibitDelay(inhibitDelayMax time.Duration) error {
	return nil
}

// Stub for the NodeShutdownManager to call
//
// It seems odd that this method is exported - seems like it should be a
// private method called by OverrideibitDelay so that the manager
// does not need to be aware of the implementation differences between
// override and reload.
func (bus *Inhibitor) ReloadLogindConf() error {
	return nil
}
