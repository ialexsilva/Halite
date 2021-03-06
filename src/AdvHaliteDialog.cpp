
//         Copyright Eóin O'Callaghan 2006 - 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "stdAfx.hpp"
#include "AdvHaliteDialog.hpp"

//#include "advtabs/ThemeTestDialog.hpp"

LRESULT AdvHaliteDialog::onInitDialog(HWND, LPARAM)
{	
	resize_class_t::DlgResize_Init(false, true, WS_CLIPCHILDREN);
	SetDlgItemText(HAL_ADVDLG_VERSION, hal::app().res_wstr(HAL_VERSION_STRING).c_str());
	
	tabCtrl_.SubclassWindow(GetDlgItem(HAL_TAB));
	
/*	mp_dlg.reset(new ThemeTestDialog());
	mp_dlg->Create(m_tabCtrl);
	
	m_tabCtrl.AddPage(*mp_dlg, L"Test1");
*/	

	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating torrent tab...")));	
	m_torrent.Create(tabCtrl_);
	tabCtrl_.AddPage(m_torrent, hal::app().res_wstr(HAL_TORRENT_TAB).c_str());
	
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating peers tab...")));	
	m_peers.Create(tabCtrl_);
	tabCtrl_.AddPage(m_peers, hal::app().res_wstr(HAL_PEERS_TAB).c_str());
	
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating files tab...")));	
	m_files.Create(tabCtrl_);
	tabCtrl_.AddPage(m_files, hal::app().res_wstr(HAL_FILES_TAB).c_str());
	
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating web seeds tab...")));	
	m_web_seeds.Create(tabCtrl_);
	tabCtrl_.AddPage(m_web_seeds, hal::app().res_wstr(HAL_WEB_SEED_TAB).c_str());
	
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating tracker tab...")));	
	m_tracker.Create(tabCtrl_);
	tabCtrl_.AddPage(m_tracker, hal::app().res_wstr(HAL_TRACKER_TAB).c_str());
	
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Creating log tab...")));	
	m_debug.Create(tabCtrl_);
	tabCtrl_.AddPage(m_debug, hal::app().res_wstr(HAL_DEBUG_TAB).c_str());
	
	tabCtrl_.SetCurrentPage(0);
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"All tabs complete!")));	
	
	return 0;
}

void AdvHaliteDialog::OnSize(UINT type, WTL::CSize)
{
	RECT rect;
	tabCtrl_.GetClientRect(&rect);
	tabCtrl_.AdjustRect(false, &rect);
	
//	mp_dlg->SetWindowPos(HWND_TOP, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE);
	
	SetMsgHandled(false);
}	

void AdvHaliteDialog::onClose()
{
	hal::event_log().post(shared_ptr<hal::EventDetail>(
		new hal::EventMsg(L"Closing Tabs!")));		

	::CloseWindow(m_torrent);
	::CloseWindow(m_peers);
	::CloseWindow(m_files);
	::CloseWindow(m_tracker);
	::CloseWindow(m_debug);

	if(::IsWindow(m_hWnd)) 
	{
		::DestroyWindow(m_hWnd);
	}
}
