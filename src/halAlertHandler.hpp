#pragma once

#define HAL_LIBTALERT_MSG(msg) \
	hal::event_log().post(boost::make_shared<hal::EventGeneral>(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()), (msg)));

#define HAL_LIBTALERT_MSG(level, msg) \
	hal::event_log().post(boost::make_shared<hal::EventGeneral>(level, convert_to_ptime(p->timestamp()), (msg)));

namespace hal
{

	template<typename M>
	void alert_msg(libt::alert* a, const M& msg)
	{
		hal::event_log().post(boost::make_shared<hal::EventGeneral>(lbt_category_to_event(a->category()), convert_to_ptime(a->timestamp()), msg));
	}

	template<typename M>
	void alert_msg(libt::alert* a, event_logger::eventLevel level, const M& msg)
	{
		hal::event_log().post(boost::make_shared<hal::EventGeneral>(level, convert_to_ptime(a->timestamp()), msg));
	}


	class AlertHandler
	{
	public:
	AlertHandler(bit_impl& bit_impl) :
		bit_impl_(bit_impl)
	{}

	torrent_internal_ptr get(libt::torrent_handle h) const
	{
		if (!h.is_valid())
			throw bit::null_torrent();

		return get(h.info_hash());
	}

	torrent_internal_ptr get(libt::sha1_hash hash) const
	{
		torrent_internal_ptr p = bit_impl_.the_torrents_.get_by_hash(hash);

		if (p)
			return p;
		else
			throw bit::null_torrent();
	}

	bool handle_alert_alt(libtorrent::alert* a)
	{
		if (auto* p = libt::alert_cast<libt::add_torrent_alert>(a))
		{
			alert_msg(a, hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_ADDED)) % get(p->handle)->name());

			get(p->handle)->set_handle(p->handle);
			get(p->handle)->process_event(new ev_added_alert((p->params.flags & libt::add_torrent_params::flag_paused) != 0, p->error));
		}
		else if (auto* p = libt::alert_cast<libt::torrent_removed_alert>(a))
		{
			alert_msg(a, hal::wform(L"Torrent removed alert %1%") % get(p->info_hash)->name());

	//		get(p->info_hash)->
		}
		else if (auto* p = libt::alert_cast<libt::save_resume_data_alert>(a))
		{
			alert_msg(a, hal::wform(hal::app().res_wstr(HAL_WRITE_RESUME_ALERT))
				% get(p->handle)->name());

			if (p->resume_data)
				get(p->handle)->write_resume_data(*p->resume_data);

			get(p->handle)->process_event(new ev_resume_data_alert());
		}
		else if (auto* p = libt::alert_cast<libt::torrent_paused_alert>(a))
		{
			wstring err = get(p->handle)->check_error();

			if (err.empty())
			{
				alert_msg(a, hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_PAUSED)) % get(p->handle)->name());

				HAL_DEV_MSG(hal::wform(L"Torrent Paused alert, %1%.") % get(p->handle)->name());

				get(p->handle)->process_event(new ev_paused_alert());
			}
			else
			{
				alert_msg(a, event_logger::warning, hal::wform(hal::app().res_wstr(HAL_TORRENT_ERROR_PAUSE_ALERT))
					% err
					% get(p->handle)->name());

				HAL_DEV_MSG(hal::wform(L"Torrent Error alert %2%, %1%.") % get(p->handle)->name() % err);

				get(p->handle)->process_event(new ev_error_alert(err));
			}
		}
		else if (auto* p = libt::alert_cast<libt::torrent_resumed_alert>(a))
		{

			alert_msg(a, hal::wform(hal::app().res_wstr(HAL_TORRENT_RESUME_ALERT)) % get(p->handle)->name());

			HAL_DEV_MSG(hal::wform(L"Torrent Resumed alert, %1%.") % get(p->handle)->name());

			get(p->handle)->process_event(new ev_resumed_alert());
		}
		else if (auto* p = libt::alert_cast<libt::torrent_finished_alert>(a))
		{
			HAL_DEV_MSG(hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FINISHED))
				% get(p->handle)->name());

			get(p->handle)->alert_finished();

			bit_impl_.signals.torrent_completed(get(p->handle)->name());
		}
		else if (auto* p = libt::alert_cast<libt::save_resume_data_failed_alert>(a))
		{
			alert_msg(a, hal::wform(hal::app().res_wstr(HAL_WRITE_RESUME_FAIL_ALERT))
				% get(p->handle)->name());

			get(p->handle)->process_event(new ev_resume_data_failed_alert());
		}
		else if (auto* p = libt::alert_cast<libt::state_changed_alert>(a))
		{
			HAL_DEV_MSG(hal::wform(L"Torrent state changed alert, %1%. From %2% -> %3%") % get(p->handle)->name() % p->prev_state % p->state);
		}
		else
		{
			alert_msg(a, event_logger::debug,
				hal::wform(hal::app().res_wstr(HAL_UNHANDLED_ALERT))
					% hal::from_utf8_safe(a->what())
					% hal::from_utf8_safe(a->message()));

			return false;
		}

		return true;
	}
		
	bool handle_alert(libtorrent::alert* a)
	{
		if (auto* p = libt::alert_cast<libt::add_torrent_alert>(a))
		{
			alert_msg(a, hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_ADDED)) % get(p->handle)->name());
			
			HAL_DEV_MSG(hal::wform(L"Torrent Added alert, %1%.") % get(p->handle)->name());

			get(p->handle)->set_handle(p->handle);
			get(p->handle)->process_event(new ev_added_alert((p->params.flags & libt::add_torrent_params::flag_paused) != 0, p->error));
		}
		else if (auto* p = libt::alert_cast<libt::external_ip_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_EXTERNAL_IP_ALERT))
						% hal::from_utf8_safe(p->message())
						% hal::from_utf8_safe(p->external_address.to_string()))
			));		

			if (!bit_impl_.use_custom_interface_ || !bit_impl_.external_interface_) 
				bit_impl_.external_interface_.reset(hal::from_utf8_safe(p->external_address.to_string()));
		}
		else if (auto* p = libt::alert_cast<libt::portmap_error_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_PORTMAP_ERROR_ALERT))
					% (p->type() == 0 ? 
						hal::app().res_wstr(HAL_PORTMAP_TYPE_PMP) : 
						hal::app().res_wstr(HAL_PORTMAP_TYPE_UPNP)))
			));				
		}
		else if (auto* p = libt::alert_cast<libt::portmap_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_PORTMAP_ALERT))
					% (p->type() == 0 ? 
						hal::app().res_wstr(HAL_PORTMAP_TYPE_PMP) : 
						hal::app().res_wstr(HAL_PORTMAP_TYPE_UPNP))
					% p->external_port)
			));				
		}
		else if (auto* p = libt::alert_cast<libt::file_error_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_FILE_ERROR_ALERT))
					% hal::from_utf8_safe(p->file)
					% hal::from_utf8_safe(p->message()))
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::dht_reply_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_DHT_REPLY_ALERT))
						% p->num_peers
						% get(p->handle)->name())
			));				
		}
		else if (auto* p = libt::alert_cast<libt::torrent_finished_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FINISHED)) 
						% get(p->handle)->name()), 
					event_logger::info, convert_to_ptime(p->timestamp()))));
		
			get(p->handle)->alert_finished();

			bit_impl_.signals.torrent_completed(get(p->handle)->name());
		}
		else if (auto* p = libt::alert_cast<libt::storage_moved_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_STOREAGE_MOVED)) 
						% get(p->handle)->name()), 
					event_logger::info, convert_to_ptime(p->timestamp()))));
		
			get(p->handle)->alert_storage_moved(from_utf8(p->path));
		}
		else if (auto* p = libt::alert_cast<libt::storage_moved_failed_alert >(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FILE_RENAME_ERR)) 
						% get(p->handle)->name()
						% p->error), 
					event_logger::warning, convert_to_ptime(p->timestamp()))));
		}
		else if (auto* p = libt::alert_cast<libt::file_renamed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FILE_RENAMED)) 
						% get(p->handle)->name()
						% p->index 
						% hal::from_utf8_safe(p->name)), 
					event_logger::debug, convert_to_ptime(p->timestamp()))));
		}
		else if (auto* p = libt::alert_cast<libt::file_rename_failed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FILE_RENAME_ERR)) 
						% get(p->handle)->name()
						% p->index), 
					event_logger::warning, convert_to_ptime(p->timestamp()))));
		}
		else if (auto* p = libt::alert_cast<libt::file_completed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_FILE_COMPLETED)) 
						% get(p->handle)->name()
						% p->index), 
					event_logger::info, convert_to_ptime(p->timestamp()))));
		
			get(p->handle)->alert_file_completed(p->index);	
		}
		else if (auto* p = libt::alert_cast<libt::metadata_received_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_METADATA)) 
						% get(p->handle)->name()), 
					event_logger::info, convert_to_ptime(p->timestamp()))));
		
			get(p->handle)->alert_metadata_completed();	
		}
		else if (auto* p = libt::alert_cast<libt::metadata_failed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_METADATA_FAILED)) 
						% get(p->handle)->name()), 
					event_logger::critical, convert_to_ptime(p->timestamp()))));
		}	
		else if (auto* p = libt::alert_cast<libt::torrent_paused_alert>(a))
		{
			wstring err = get(p->handle)->check_error();

			if (err.empty())
			{
				event_log().post(shared_ptr<EventDetail>(
					new EventMsg((hal::wform(hal::app().res_wstr(LBT_EVENT_TORRENT_PAUSED)) 
							% get(p->handle)->name()), 
						event_logger::debug, convert_to_ptime(p->timestamp()))));
			
				HAL_DEV_MSG(hal::wform(L"Torrent Paused alert, %1%.") % get(p->handle)->name());

				get(p->handle)->process_event(new ev_paused_alert());
			}
			else
			{
				HAL_DEV_MSG(hal::wform(L"Torrent Error alert %2%, %1%.") % get(p->handle)->name() % err);

				event_log().post(shared_ptr<EventDetail>(
					new EventMsg((hal::wform(hal::app().res_wstr(HAL_TORRENT_ERROR_PAUSE_ALERT)) 
							% err 
							% get(p->handle)->name()), 
						event_logger::warning, convert_to_ptime(p->timestamp()))));

				get(p->handle)->process_event(new ev_error_alert(err));
			}
		}	
		else if (auto* p = libt::alert_cast<libt::torrent_resumed_alert>(a))
		{

			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(HAL_TORRENT_RESUME_ALERT)) 
						% get(p->handle)->name()), 
					event_logger::debug, convert_to_ptime(p->timestamp()))));

			HAL_DEV_MSG(hal::wform(L"Torrent Resumed alert, %1%.") % get(p->handle)->name());

			get(p->handle)->process_event(new ev_resumed_alert());
		}	
		else if (auto* p = libt::alert_cast<libt::save_resume_data_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(HAL_WRITE_RESUME_ALERT)) 
						% get(p->handle)->name()), 
					event_logger::info, convert_to_ptime(p->timestamp()))));

			if (p->resume_data)
				get(p->handle)->write_resume_data(*p->resume_data);

			get(p->handle)->process_event(new ev_resume_data_alert());
		}	
		else if (auto* p = libt::alert_cast<libt::save_resume_data_failed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(HAL_WRITE_RESUME_FAIL_ALERT)) 
						% get(p->handle)->name()), 
					event_logger::warning, convert_to_ptime(p->timestamp()))));

			get(p->handle)->process_event(new ev_resume_data_failed_alert());
		}	
		else if (auto* p = libt::alert_cast<libt::peer_error_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_PEER_ALERT))
						% hal::from_utf8_safe(p->message())
						% hal::from_utf8_safe(p->ip.address().to_string()))
			));				
		}		
		else if (auto* p = libt::alert_cast<libt::peer_ban_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_PEER_BAN_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->ip.address().to_string()))
			));				
		}		
		else if (auto* p = libt::alert_cast<libt::hash_failed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_HASH_FAIL_ALERT))
						% get(p->handle)->name()
						% p->piece_index)
			));				
		}		
		else if (auto* p = libt::alert_cast<libt::url_seed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_URL_SEED_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->url)
						% hal::from_utf8_safe(p->message()))
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::tracker_warning_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_TRACKER_WARNING_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->message()))
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::tracker_announce_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventMsg((hal::wform(hal::app().res_wstr(HAL_TRACKER_ANNOUNCE_ALERT)) 
						% get(p->handle)->name()), 
					event_logger::info, convert_to_ptime(p->timestamp()))));
		}	
		else if (auto* p = libt::alert_cast<libt::tracker_error_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_TRACKER_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->message())
						% p->times_in_row
						% p->status_code)
			));				

			hal::peer_details_vec peers;
			get(p->handle)->get_peer_details(peers);

			HAL_DEV_MSG(hal::wform(L"Recieved %1% peers") % peers.size());
			for (const auto& peer : peers)
			{			
				HAL_DEV_MSG(hal::wform(L"  --  %1% : %2%") % peer.ip_address % peer.port);
			}
		}	
		else if (auto* p = libt::alert_cast<libt::scrape_failed_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_TRACKER_SCRAPE_FAILED_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->message()))
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::tracker_reply_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_TRACKER_REPLY_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->message())
						% p->num_peers)
			));				
		}
	
		else if (auto* p = libt::alert_cast<libt::fastresume_rejected_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(lbt_category_to_event(p->category()), convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_FAST_RESUME_ALERT))
						% get(p->handle)->name()
						% hal::from_utf8_safe(p->message()))
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::piece_finished_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_PIECE_FINISHED_ALERT))
						% get(p->handle)->name()
						% p->piece_index)
			));				
		}	
		else if (auto* p = libt::alert_cast<libt::block_finished_alert>(a))
		{
	/*		event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_BLOCK_FINISHED_ALERT))
						% get(p->handle)->name()
						% p->block_index
						% p->piece_index)
			));				
	*/	}	
		else if (auto* p = libt::alert_cast<libt::block_downloading_alert>(a))
		{
	/*		event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_BLOCK_DOWNLOADING_ALERT))
						% get(p->handle)->name()
						% p->block_index
						% p->piece_index)
			));				
	*/	}	
		else if (auto* p = libt::alert_cast<libt::listen_failed_alert>(a))
		{
			if (p->endpoint.address().is_v6())
			{	
				event_log().post(shared_ptr<EventDetail>(
					new EventGeneral(event_logger::info, convert_to_ptime(p->timestamp()),
						hal::app().res_wstr(HAL_LISTEN_V6_FAILED_ALERT))
				));		
			}
			else
			{
				event_log().post(shared_ptr<EventDetail>(
					new EventGeneral(event_logger::info, convert_to_ptime(p->timestamp()),
						hal::wform(hal::app().res_wstr(HAL_LISTEN_FAILED_ALERT))
							% hal::from_utf8_safe(p->message()))
				));
			}
		}	
		else if (auto* p = libt::alert_cast<libt::listen_succeeded_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::info, convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_LISTEN_SUCCEEDED_ALERT))
						% hal::from_utf8_safe(p->message()))
			)	);	

			//bit_impl_.signals.successful_listen();
		}	
		else if (auto* p = libt::alert_cast<libt::portmap_log_alert>(a))
		{ 
		
		}	
		else if (auto* p = libt::alert_cast<libt::dht_pkt_alert>(a))
		{ 
		
		}
		else if (auto* p = libt::alert_cast<libt::dht_log_alert>(a))
		{ 
		
		}
		else if (auto* p = libt::alert_cast<libt::log_alert>(a))
		{ 
			// Nothing for now

		/*	event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(a->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_LOG_ALERT))
						% hal::from_utf8_safe(a->message()))
			));	
		*/
		}
		else if (auto* p = libt::alert_cast<libt::peer_blocked_alert>(a))
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(p->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_IPFILTER_ALERT))
					% hal::from_utf8_safe(p->ip.to_string())
					% hal::from_utf8_safe(p->message()))
				));
		}
		else if (auto* p = libt::alert_cast<libt::peer_connect_alert>(a))
		{
			// Nothing for now
		}
		else if (auto* p = libt::alert_cast<libt::peer_disconnected_alert>(a))
		{
			// Nothing for now
		}
		else if (auto* p = libt::alert_cast<libt::peer_log_alert>(a))
		{
			// Nothing for now
		}
		else if (auto* p = libt::alert_cast<libt::torrent_log_alert>(a))
		{
			// Nothing for now
		}
		else if (auto* p = libt::alert_cast<libt::picker_log_alert>(a))
		{
			// Nothing for now
		}
		else 
		{
			event_log().post(shared_ptr<EventDetail>(
				new EventGeneral(event_logger::debug, convert_to_ptime(a->timestamp()),
					hal::wform(hal::app().res_wstr(HAL_UNHANDLED_ALERT))
					% hal::from_utf8_safe(a->what())
					% hal::from_utf8_safe(a->message()))
			));	

			return true;
		}

		return true;
	}

	private:
		bit_impl& bit_impl_;
			
	};

	}

