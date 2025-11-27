#pragma once

#include <components/bk_player_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief      Create a player handle.
 *              Supports creating a player that plays directly. For configuration, please refer to DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG().
 *              Supports creating a player that does not play directly. The decoded PCM data can be output to other components for playback. For configuration, please refer to DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG().
 *
 * @param[in]      cfg  The player configuration.
 *
 * @return         The player handle.
 *                 - Not NULL: Success.
 *                 - NULL: Failed.
 */
bk_player_handle_t bk_player_create(bk_player_cfg_t *cfg);

/**
 * @brief      Destroy a player handle.
 *
 * @param[in]      player_handle  The player handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_destroy(bk_player_handle_t player_handle);

/**
 * @brief      Start a player.
 *
 * @param[in]      player_handle  The player handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_start(bk_player_handle_t player_handle);

/**
 * @brief      Stop a player.
 *
 * @param[in]      player_handle  The player handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_stop(bk_player_handle_t player_handle);

/**
 * @brief      Set the URI of a player.
 *
 * @param[in]      player_handle  The player handle.
 * @param[in]      uri_info       The uri information
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_set_uri(bk_player_handle_t player_handle, player_uri_info_t *uri_info);

/**
 * @brief      Get the state of a player.
 *
 * @param[in]      player_handle  The player handle.
 * @param[out]     state         The pointer to store the player state.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_get_state(bk_player_handle_t player_handle, bk_player_state_t *state);

/**
 * @brief      Set the decode type of a player.
 *
 * @param[in]      player_handle  The player handle.
 * @param[in]      dec_type       The decode type.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_set_decode_type(bk_player_handle_t player_handle, audio_dec_type_t dec_type);

/**
 * @brief      Set the output port of a player.
 *             Use this interface to set the output port when the created player is not playing data.
 *             Do not use this interface to set the output port when the player is playing data.
 *
 * @param[in]      player_handle  The player handle.
 * @param[in]      port           The output port handle.
 * @param[in]      port_type      The output port type.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_player_set_output_port(bk_player_handle_t player_handle, audio_port_handle_t port);

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */