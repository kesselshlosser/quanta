<?php

/**
 * Class NodeFactory
 * This factory class is used for loading nodes, etc.
 */
class NodeFactory {

  /**
   * Load a node, by checking access and running hooks.
   * @param $env
   * @param $node
   */
  public static function load($env, $node, $tpl = NULL) {
    $node = new Node($env, $node);
    $env->hook('node_open', array('node' => &$node), NULL, $tpl);
    return $node;
  }

  /**
   * Create a "Forbidden" empty node.
   * @param $env
   * @param $node
   */
  public static function buildForbiddenNode($env) {
    $node = new Node($env, NULL);
    $node->forbidden = TRUE;
    $node->setBody('FORBIDDEN');
    return $node;
  }


  /**
   * Create an empty node.
   * @param $env
   * @param $node
   */
  public static function buildEmptyNode($env, $father) {
    $node = new Node($env, NODE_NEW, $father);
    return $node;
  }


  /**
   * Gets the current viewed node.
   * @return bool
   */
  public static function current($env) {
    $tpl = isset($_REQUEST['tpl']) ? $_REQUEST['tpl'] : NULL;
    if ($env->getContext() == NODE_ACTION_ADD) {
      $node = NodeFactory::buildEmptyNode($env, $env->getRequestedPath());
    } else {
      $node = NodeFactory::load($env, $env->getRequestedPath(), $tpl);
    }
    return $node;
  }

  /**
   * Request to perform an action on the node. Check permissions and
   * execute the action accordingly.
   * @param $env Environment
   * @param $action string
   * @return string
   */
  public static function requestAction($env, $action, $nodedata) {
    $response = new StdClass();

    // TODO: circular dependency to user.
    $user = User::current($env);

    // TODO: refactor this stuff.
    // When user didn't enter a path...
    if (trim($nodedata['edit-path']) == '') {
      $path = $env->getCandidatePath($nodedata['edit-title']);
    }
    else {
      $path = $nodedata['edit-path'];
    }

		$father = ($action == NODE_ACTION_ADD) ? $nodedata['edit-father'] : NULL;
    $node = new Node($env, $path, $father);

    switch ($action) {
      case NODE_ACTION_ADD:
      case NODE_ACTION_EDIT:
        if ($action == NODE_ACTION_ADD) {
					$node->realpath = $node->father->realpath . '/' . $path;
          $node->setAuthor($user->getName());
        }
        // TODO complete validation code.
        if (isset($nodedata['edit-thumbnail'])) {
          $node->setThumbnail($nodedata['edit-thumbnail']);
        }
        $datetime = strtotime($nodedata['edit-date'] . ' ' . $nodedata['edit-time']);
        $has_access = ($user->checkAccess(($action == NODE_ACTION_ADD) ? $node->father : $node, $action));
        if (TRUE) {
          $node->setTitle($nodedata['edit-title']);
          $node->setBody($nodedata['edit-content']);
          $node->setAuthor($nodedata['edit-author']);
          $node->setTeaser($nodedata['edit-teaser']);
          $node->setContent('<h1>' . $node->getTitle() . '</h1>' . $node->getBody());
          $node->setTimestamp($datetime > 0 ? $datetime : time());
          $node->tmp_files_dir = $nodedata['tmp_files_dir'];

          $env->hook('node_presave', array(
            'node' => &$node,
            'data' => $nodedata,
            'action' => $action,
          ));

          if ($node->validate()) {
            $node->save();
            $env->hook('node_save', array(
              'node' => &$node,
              'data' => $nodedata
            ));
            $response->redirect = '/' . $node->getName() . '/';
          }
          else {
            // TODO: make this good.
            $response->errors = Message::burnMessages();
          }
        }
        else {
          $response->redirect = '/403';
        }
        break;

      case NODE_ACTION_DELETE:
        $has_access = ($user->checkAccess($node, NODE_ACTION_DELETE));
        if ($has_access) {
          $node->delete();
          new Message($node->env, $node->getName() . ' was deleted correctly');
          $response->redirect = '/' . $node->getFather()->getName() . '/';
        }
        break;
    }
    $response_json = json_encode($response);
    return $response_json;
  }

}